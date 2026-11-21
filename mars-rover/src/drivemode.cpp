#include "drivemode.h"
#include <Arduino.h>
#include "config.h"
#include "distance.h"
#include "light.h"
#include "motors.h"
#include "netlog.h"
#include "power.h"

namespace drivemode {

namespace {

// Forward/Backup/Turn are shared by Explore and Seek. Scan/Align are Seek only.
enum class Phase { Forward, Backup, Turn, Scan, Align };

Mode mode = Mode::Stop;
Phase phase = Phase::Forward;
uint32_t phaseStart = 0, lastReadMs = 0;
int turnDir = 1;       // +1 spins right, -1 spins left
uint8_t failures = 0;  // consecutive failed sensor reads
bool paused = false;   // motors stopped because of a failed read

// Seek state
float steerL = 1, steerR = 1;     // forward-phase wheel scaling toward the brighter side
float smoothAvg = 0, smoothDiff = 0;
bool smoothInit = false;
float bestAvg = 0, scanBest = 0;  // lux
uint32_t bestAt = 0;

void log(const char* fmt, float a = 0) {
  char ts[16];
  netlog::stamp(ts, sizeof(ts));
  netlog::out.printf("%s [%s] ", ts, mode == Mode::Seek ? "seek" : "explore");
  netlog::out.printf(fmt, a);
  netlog::out.println();
}

void applyPhase() {
  float v = mode == Mode::Seek ? SEEK_SPEED : EXPLORE_SPEED;
  switch (phase) {
    case Phase::Forward: motors::set(v * steerL, v * steerR); break;
    case Phase::Backup: motors::set(-EXPLORE_SPEED, -EXPLORE_SPEED); break;
    default: motors::set(turnDir * EXPLORE_TURN_SPEED, -turnDir * EXPLORE_TURN_SPEED); break;  // spin
  }
}

void enter(Phase p, uint32_t now) {
  phase = p;
  phaseStart = now;
  applyPhase();
}

void finish() {
  motors::stop();
  mode = Mode::Stop;
  power::setForced(false);
}

void enterForward(uint32_t now) {
  steerL = steerR = 1;
  enter(Phase::Forward, now);
}

}  // namespace

const char* name() {
  switch (mode) {
    case Mode::Explore: return "explore";
    case Mode::Seek: return "seek";
    case Mode::Sleep: return "sleep";
    default: return "stop";
  }
}

Mode current() { return mode; }
bool active() { return mode == Mode::Explore || mode == Mode::Seek; }

bool set(Mode m, uint32_t now) {
  if ((m == Mode::Explore || m == Mode::Seek) && !distance::ready()) return false;
  if (m == Mode::Seek && !light::ready()) return false;
  if (m == mode) return true;
  motors::stop();
  mode = m;
  power::setForced(m == Mode::Sleep);
  if (m == Mode::Explore || m == Mode::Seek) {
    failures = 0;
    paused = false;
    lastReadMs = now - EXPLORE_READ_MS;  // read right away
    steerL = steerR = 1;
  }
  if (m == Mode::Explore) {
    enter(Phase::Forward, now);
    log("start, buffer %.2f in", distance::bufferIn());
  } else if (m == Mode::Seek) {
    smoothInit = false;
    scanBest = 0;
    turnDir = (esp_random() & 1) ? 1 : -1;
    enter(Phase::Scan, now);
    log("start, scanning for the brightest direction (buffer %.2f in)", distance::bufferIn());
  }
  return true;
}

void loop(uint32_t now) {
  if (!active()) return;

  if (phase == Phase::Backup && !paused) {
    if (now - phaseStart >= EXPLORE_BACKUP_MS) {
      turnDir = (esp_random() & 1) ? 1 : -1;
      enter(Phase::Turn, now);
    }
    return;  // no sensor behind: reverse blind for a short, fixed time
  }
  if (now - lastReadMs < EXPLORE_READ_MS) return;
  lastReadMs = now;

  // Spinning in place (Scan/Align) needs no distance; Forward/Turn do. Seek needs light except while turning
  // away from an obstacle.
  int mm = 0;
  float l = 0, r = 0;
  bool needDist = phase == Phase::Forward || phase == Phase::Turn;
  bool needLight = mode == Mode::Seek && (phase == Phase::Forward || phase == Phase::Scan || phase == Phase::Align);
  bool ok = true;
  if (needDist) {
    mm = distance::readMm();
    ok = mm >= 0;
  }
  if (ok && needLight) ok = light::read(l, r);
  if (!ok) {
    motors::stop();  // never drive on a bad reading
    paused = true;
    if (++failures >= 3) {
      log("sensor failing, stopping");
      finish();
    }
    return;
  }
  failures = 0;
  if (paused) {
    paused = false;
    applyPhase();
  }

  float avg = (l + r) / 2;
  float clear = needDist ? distance::clearanceIn(mm) : 0;

  switch (phase) {
    case Phase::Scan:
      scanBest = max(scanBest, avg);
      if (now - phaseStart >= SEEK_SCAN_MS) {
        log("scan done, brightest %.0f lux, turning back to it", scanBest);
        enter(Phase::Align, now);
      }
      break;

    case Phase::Align:
      if (avg >= SEEK_ALIGN_FRAC * scanBest || now - phaseStart >= SEEK_SCAN_MS) {
        log("aligned at %.0f lux, driving", avg);
        smoothInit = false;
        bestAvg = avg;
        bestAt = now;
        enterForward(now);
      }
      break;

    case Phase::Forward:
      if (clear < distance::bufferIn()) {
        log("obstacle, %.1f in clear, backing up", clear);
        enter(Phase::Backup, now);
        break;
      }
      if (mode != Mode::Seek) break;
      if (!smoothInit) {
        smoothAvg = avg;
        smoothDiff = l - r;
        smoothInit = true;
      } else {
        smoothAvg += SEEK_SMOOTH_ALPHA * (avg - smoothAvg);
        smoothDiff += SEEK_SMOOTH_ALPHA * ((l - r) - smoothDiff);
      }
      {
        float band = max(SEEK_BAND_MIN_LUX, SEEK_BAND_FRAC * smoothAvg);
        // Brighter on one side: slow that side's wheels so the rover arcs toward it.
        steerL = smoothDiff > band ? 1 - SEEK_ARC : 1;
        steerR = smoothDiff < -band ? 1 - SEEK_ARC : 1;
        applyPhase();
      }
      if (smoothAvg > bestAvg * SEEK_IMPROVE) {
        bestAvg = smoothAvg;
        bestAt = now;
      } else if (now - bestAt >= SEEK_PLATEAU_MS) {
        log("brightness stopped improving at %.0f lux, parked", smoothAvg);
        finish();
      }
      break;

    case Phase::Turn:
      if (now - phaseStart >= EXPLORE_TURN_MIN_MS && clear >= distance::bufferIn() + EXPLORE_CLEAR_MARGIN_IN) {
        log("clear, %.1f in ahead, driving", clear);
        enterForward(now);
      } else if (now - phaseStart >= EXPLORE_TURN_MAX_MS) {
        log("boxed in, stopping");
        finish();
      }
      break;

    default: break;
  }
}

}  // namespace drivemode
