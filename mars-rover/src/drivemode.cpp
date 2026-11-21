#include "drivemode.h"
#include <Arduino.h>
#include "config.h"
#include "distance.h"
#include "motors.h"
#include "netlog.h"
#include "power.h"

namespace drivemode {

namespace {

enum class Phase { Forward, Backup, Turn };

Mode mode = Mode::Stop;
Phase phase = Phase::Forward;
uint32_t phaseStart = 0, lastReadMs = 0;
int turnDir = 1;       // +1 spins right, -1 spins left
uint8_t failures = 0;  // consecutive failed distance reads
bool paused = false;   // motors stopped because of a failed read

void log(const char* fmt, float a = 0) {
  char ts[16];
  netlog::stamp(ts, sizeof(ts));
  netlog::out.printf("%s [explore] ", ts);
  netlog::out.printf(fmt, a);
  netlog::out.println();
}

void applyPhase() {
  switch (phase) {
    case Phase::Forward: motors::set(EXPLORE_SPEED, EXPLORE_SPEED); break;
    case Phase::Backup: motors::set(-EXPLORE_SPEED, -EXPLORE_SPEED); break;
    case Phase::Turn: motors::set(turnDir * EXPLORE_TURN_SPEED, -turnDir * EXPLORE_TURN_SPEED); break;
  }
}

void enter(Phase p, uint32_t now) {
  phase = p;
  phaseStart = now;
  applyPhase();
}

}  // namespace

const char* name() {
  switch (mode) {
    case Mode::Explore: return "explore";
    case Mode::Sleep: return "sleep";
    default: return "stop";
  }
}

Mode current() { return mode; }

bool set(Mode m, uint32_t now) {
  if (m == Mode::Explore && !distance::ready()) return false;
  if (m == mode) return true;
  motors::stop();
  mode = m;
  power::setForced(m == Mode::Sleep);
  if (m == Mode::Explore) {
    failures = 0;
    paused = false;
    lastReadMs = now - EXPLORE_READ_MS;  // read right away
    enter(Phase::Forward, now);
    log("start, buffer %.2f in", distance::bufferIn());
  }
  return true;
}

void loop(uint32_t now) {
  if (mode != Mode::Explore) return;

  if (phase == Phase::Backup && !paused) {
    if (now - phaseStart >= EXPLORE_BACKUP_MS) {
      turnDir = (esp_random() & 1) ? 1 : -1;
      enter(Phase::Turn, now);
    }
    return;  // no sensor behind: reverse blind for a short, fixed time
  }
  if (now - lastReadMs < EXPLORE_READ_MS) return;
  lastReadMs = now;

  int mm = distance::readMm();
  if (mm < 0) {
    motors::stop();  // never drive on a bad reading
    paused = true;
    if (++failures >= 3) {
      log("distance sensor failing, stopping");
      mode = Mode::Stop;
      power::setForced(false);
    }
    return;
  }
  failures = 0;
  if (paused) {
    paused = false;
    applyPhase();
  }

  float clear = distance::clearanceIn(mm);
  if (phase == Phase::Forward) {
    if (clear < distance::bufferIn()) {
      log("obstacle, %.1f in clear, backing up", clear);
      enter(Phase::Backup, now);
    }
  } else if (phase == Phase::Turn) {
    uint32_t t = now - phaseStart;
    if (t >= EXPLORE_TURN_MIN_MS && clear >= distance::bufferIn() + EXPLORE_CLEAR_MARGIN_IN) {
      log("clear, %.1f in ahead, driving", clear);
      enter(Phase::Forward, now);
    } else if (t >= EXPLORE_TURN_MAX_MS) {
      log("boxed in, stopping");
      motors::stop();
      mode = Mode::Stop;
      power::setForced(false);
    }
  }
}

}  // namespace drivemode
