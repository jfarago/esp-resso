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

// Forward/Backup/Look/Turn are shared by Explore and Seek. Scan/Align are Seek only.
enum class Phase { Forward, Backup, Look, Turn, Scan, Align };

Mode mode = Mode::Stop;
Phase phase = Phase::Forward;
uint32_t phaseStart = 0, lastReadMs = 0;
int turnDir = 1;       // +1 spins right, -1 spins left
int lastTurnDir = 1;   // direction of the previous avoidance turn, to bias away from repeating it
uint32_t turnMinMs = EXPLORE_TURN_MIN_MS;  // minimum length of the current turn
// Look phase: a short sweep right then left, recording the most clearance seen on each side.
float lookBestR = 0, lookBestL = 0;
// Oscillation guard: obstacles hit in quick succession mean it is bouncing between things.
uint8_t quickHits = 0;
uint8_t boxedTries = 0;  // consecutive turns that timed out without finding a way out
uint32_t lastObstacleAt = 0;
uint8_t failures = 0;  // consecutive failed sensor reads
bool paused = false;   // motors stopped because of a failed read

// Seek state
float steerL = 1, steerR = 1;     // forward-phase wheel scaling toward the brighter side
float smoothAvg = 0, smoothDiff = 0;
bool smoothInit = false;
float arc = SEEK_ARC;
float bestAvg = 0, scanBest = 0;  // lux
uint32_t bestAt = 0, lastLogMs = 0;

// Stuck detection: distance reading when the current "no progress" window started.
bool stuckInit = false;
int stuckRefMm = 0;
uint32_t stuckAt = 0;
int prevMm = -1;  // previous forward reading, for climb detection (-1 = none)

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
    case Phase::Look: motors::set(turnDir * EXPLORE_TURN_SPEED, -turnDir * EXPLORE_TURN_SPEED); break;
    default: motors::set(turnDir * EXPLORE_TURN_SPEED, -turnDir * EXPLORE_TURN_SPEED); break;  // spin
  }
}

void enter(Phase p, uint32_t now) {
  phase = p;
  phaseStart = now;
  stuckInit = false;
  prevMm = -1;
  applyPhase();
}

void finish() {
  motors::stop();
  mode = Mode::Stop;
  power::setForced(false);
}

// Called on each obstacle. Hitting several in a row within a short time means it is ping-ponging between
// them, so the next turn is made much longer to head off somewhere new.
void noteObstacle(uint32_t now) {
  quickHits = (lastObstacleAt && now - lastObstacleAt < EXPLORE_QUICK_HIT_MS) ? quickHits + 1 : 0;
  lastObstacleAt = now;
  if (quickHits >= EXPLORE_QUICK_HITS) {
    log("bouncing between obstacles, making a long turn");
    quickHits = 0;
    turnMinMs = EXPLORE_ESCAPE_TURN_MS;
  } else {
    turnMinMs = EXPLORE_TURN_MIN_MS;
  }
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
float seekArc() { return arc; }
void setSeekArc(float a) { arc = a; }
bool active() { return mode == Mode::Explore || mode == Mode::Seek; }

bool set(Mode m, uint32_t now) {
  if ((m == Mode::Explore || m == Mode::Seek) && m != mode) power::sensorsWake();
  if ((m == Mode::Explore || m == Mode::Seek) && !distance::ready()) return false;
  if (m == Mode::Seek && !light::ready()) return false;
  if (m == mode) return true;
  motors::stop();
  mode = m;
  power::setForced(m == Mode::Sleep);
  if (m == Mode::Explore || m == Mode::Seek) {
    failures = 0;
    boxedTries = 0;
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
      turnDir = 1;  // sweep right first
      lookBestR = lookBestL = -1000;
      enter(Phase::Look, now);
    }
    return;  // no sensor behind: reverse blind for a short, fixed time
  }
  if (now - lastReadMs < EXPLORE_READ_MS) return;
  lastReadMs = now;

  // Spinning in place (Scan/Align) needs no distance; Forward/Turn do. Seek needs light except while turning
  // away from an obstacle.
  int mm = 0;
  float l = 0, r = 0;
  bool needDist = phase == Phase::Forward || phase == Phase::Look || phase == Phase::Turn;
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
    stuckInit = false;  // motors were stopped, restart the no-progress window
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
      if (boxedTries && now - phaseStart >= EXPLORE_BOXED_RESET_MS) boxedTries = 0;  // got away, forget it
      if (clear < distance::bufferIn()) {
        log("obstacle, %.1f in clear, backing up", clear);
        noteObstacle(now);
        enter(Phase::Backup, now);
        break;
      }
      // Climbing: driving forward only ever shortens the distance, so a sudden jump up from a near reading
      // means the nose has tipped up a wall and the sensor is now looking at the ceiling (or nothing).
      if (prevMm >= 0 && prevMm < CLIMB_NEAR_MM && mm - prevMm >= CLIMB_JUMP_MM) {
        log("nose tipped up (%.0f mm jump), backing off the wall", float(mm - prevMm));
        noteObstacle(now);
        enter(Phase::Backup, now);
        break;
      }
      prevMm = mm;
      // Stuck: wheels driving but the distance ahead isn't changing. An out-of-range reading can't show
      // progress, so it just restarts the window.
      if (mm == distance::OUT_OF_RANGE_MM || !stuckInit || abs(mm - stuckRefMm) >= STUCK_MIN_MM) {
        stuckInit = mm != distance::OUT_OF_RANGE_MM;
        stuckRefMm = mm;
        stuckAt = now;
      } else if (now - stuckAt >= STUCK_MS) {
        log("stuck, no progress for %.1f s, backing up", (now - stuckAt) / 1000.0f);
        noteObstacle(now);
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
        // The harder the imbalance, the harder the turn: ramps from 0 at the band edge to full `arc`.
        float range = max(2 * SEEK_BAND_MIN_LUX, SEEK_STEER_RANGE_FRAC * smoothAvg);
        float s = constrain((fabsf(smoothDiff) - band) / range, 0.0f, 1.0f);
        steerL = smoothDiff > 0 ? 1 - arc * s : 1;
        steerR = smoothDiff < 0 ? 1 - arc * s : 1;
        applyPhase();
      }
      if (now - lastLogMs >= SEEK_LOG_MS) {
        lastLogMs = now;
        char ts[16];
        netlog::stamp(ts, sizeof(ts));
        netlog::out.printf("%s [seek] L %.0f R %.0f lux, avg %.0f, best %.0f, wheels %.0f%%/%.0f%%\n", ts, l, r,
                           smoothAvg, bestAvg, steerL * 100, steerR * 100);
      }
      if (smoothAvg > bestAvg * SEEK_IMPROVE) {
        bestAvg = smoothAvg;
        bestAt = now;
      } else if (now - bestAt >= SEEK_PLATEAU_MS) {
        log("brightness stopped improving at %.0f lux, parked", smoothAvg);
        finish();
      }
      break;

    case Phase::Look: {
      // Right for LOOK_MS, then left for twice that (passing back through center), keeping the best
      // clearance seen on each side. Only the far half of the left leg counts as "left".
      uint32_t t = now - phaseStart;
      if (t < EXPLORE_LOOK_MS) {
        lookBestR = max(lookBestR, clear);
      } else if (t < 3 * EXPLORE_LOOK_MS) {
        if (turnDir != -1) {
          turnDir = -1;
          applyPhase();
        }
        if (t >= 2 * EXPLORE_LOOK_MS) lookBestL = max(lookBestL, clear);
      } else {
        float diff = lookBestL - lookBestR;
        if (fabsf(diff) < EXPLORE_LOOK_TIE_IN) {
          turnDir = -lastTurnDir;  // no clear winner: alternate so it doesn't keep favoring one side
        } else {
          turnDir = diff > 0 ? -1 : 1;
        }
        lastTurnDir = turnDir;
        log(turnDir > 0 ? "more room to the right (%.1f in), turning right" : "more room to the left (%.1f in), turning left",
            turnDir > 0 ? lookBestR : lookBestL);
        enter(Phase::Turn, now);
      }
      break;
    }

    case Phase::Turn:
      // Each failed attempt lowers the bar for "clear" (margin / (1 + tries)) so a tight corner can still be left.
      if (now - phaseStart >= turnMinMs &&
          clear >= distance::bufferIn() + EXPLORE_CLEAR_MARGIN_IN / (1 + boxedTries)) {
        log("clear, %.1f in ahead, driving", clear);
        enterForward(now);
      } else if (now - phaseStart >= EXPLORE_TURN_MAX_MS) {
        if (++boxedTries > EXPLORE_BOXED_RETRIES) {
          log("boxed in, no way out after retries, stopping");
          boxedTries = 0;
          finish();
        } else {
          log("boxed in, backing up to try again (%.0f)", float(boxedTries));
          enter(Phase::Backup, now);
        }
      }
      break;

    default: break;
  }
}

}  // namespace drivemode
