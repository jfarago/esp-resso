#include "commands.h"
#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>
#include "config.h"
#include "distance.h"
#include "drivemode.h"
#include "light.h"
#include "motors.h"
#include "netlog.h"
#include "power.h"

namespace commands {

namespace {

bool moving = false;
uint32_t moveStart = 0, moveMs = 0;
float curL = 0, curR = 0;
bool wasConnected = false;
float ipsFull = INCHES_PER_SEC_FULL;  // inches per second at 100% power
float odoL = 0, odoR = 0;             // inches per side, signed (forward positive)
float odoTotal = 0;                   // inches travelled regardless of direction
Preferences prefs;

// Adds the in-progress move (as far as it got) to the odometer.
void account(uint32_t now) {
  if (!moving) return;
  uint32_t el = min(now - moveStart, moveMs);
  float s = el / 1000.0f;
  odoL += curL * ipsFull * s;
  odoR += curR * ipsFull * s;
  odoTotal += (fabsf(curL) + fabsf(curR)) / 2 * ipsFull * s;
  moveMs -= el;  // rebase so the same time is never counted twice
  moveStart = now;
}

void startMove(uint32_t now, float l, float r, uint32_t ms) {
  account(now);
  motors::set(l, r);
  curL = l;
  curR = r;
  moveStart = now;
  moveMs = ms;
  moving = true;
}

// Odometer lives in NVS ("rover" namespace), which a normal flash does not erase. Saved when a move ends.
void saveOdo() {
  prefs.putFloat("odoL", odoL);
  prefs.putFloat("odoR", odoR);
  prefs.putFloat("odoT", odoTotal);
}

void stopMove() {
  bool was = moving;
  account(millis());
  motors::stop();
  curL = curR = 0;
  moving = false;
  if (was) saveOdo();
}

void help() {
  netlog::out.println(
      "commands (speed 0-100 %, ms = duration in milliseconds, max " + String(MOVE_MAX_MS) + "):\n"
      "  f <speed> <ms>          both sides forward\n"
      "  b <speed> <ms>          both sides backward\n"
      "  l <speed> <ms>          left side only (negative = reverse), right stopped\n"
      "  r <speed> <ms>          right side only (negative = reverse), left stopped\n"
      "  m <left> <right> <ms>   both sides, signed -100..100\n"
      "  d <speed> <inches>      drive a distance (negative inches = backward), uses calibration\n"
      "  cal <speed> <ms> <in>   calibrate: you drove <speed>% for <ms> and it went <in> inches\n"
      "  odo [reset]             distance travelled estimate\n"
      "  dist                    distance ahead from the VL53L0X\n"
      "  lux                     left/right light levels from the BH1750s\n"
      "  cfg [reset]             show all saved settings (cal, buf, arc); reset restores defaults\n"
      "  arc [0-2]               show/set how sharply seek steers toward light (saved)\n"
      "  i2c                     scan the I2C bus (0x23/0x5C = BH1750s, 0x29 = VL53L0X)\n"
      "  buf [inches]            show/set the clearance kept from obstacles (saved)\n"
      "  explore                 drive around, backing up and turning to avoid obstacles\n"
      "  seek                    find the brightest spot (to recharge), then stop\n"
      "  deepsleep <minutes>     deep sleep (max 60), then reboot; unreachable until it wakes\n"
      "  stop (or s)             stop motors, leave explore/sleep\n"
      "  sleep                   light sleep until stop/explore/a move command\n"
      "  ?                       this list and motor state");
}

bool parseFloat(const char* tok, float& v) {
  if (!tok) return false;
  char* end;
  v = strtof(tok, &end);
  return end != tok && *end == 0 && isfinite(v);
}

void printOdo() {
  netlog::out.printf("odo: net %.1f in (L %.1f, R %.1f), total %.1f in, cal %.2f in/s at 100%%\n",
                     (odoL + odoR) / 2, odoL, odoR, odoTotal, ipsFull);
}

bool parseInt(const char* tok, long& v) {
  if (!tok) return false;
  char* end;
  v = strtol(tok, &end, 10);
  return end != tok && *end == 0;
}

void handle(const char* line, uint32_t now) {
  char buf[96];
  strlcpy(buf, line, sizeof(buf));
  char* save;
  char* cmd = strtok_r(buf, " \t", &save);
  if (!cmd) return;
  char* args[3];
  int n = 0;
  while (n < 3 && (args[n] = strtok_r(nullptr, " \t", &save))) n++;
  if (strtok_r(nullptr, " \t", &save)) n = 99;  // too many arguments

  if (!strcmp(cmd, "?")) {
    help();
    netlog::out.printf("state: %s L=%.0f%% R=%.0f%%, mode %s\n", moving ? "moving" : "stopped",
                       curL * 100, curR * 100, drivemode::name());
    return;
  }
  if (!strcmp(cmd, "s") || !strcmp(cmd, "stop")) {
    if (n != 0) { netlog::out.println("err: usage: stop"); return; }
    stopMove();
    drivemode::set(drivemode::Mode::Stop, now);
    netlog::out.println("ok stop");
    return;
  }
  if (!strcmp(cmd, "explore")) {
    if (n != 0) { netlog::out.println("err: usage: explore"); return; }
    stopMove();
    if (drivemode::set(drivemode::Mode::Explore, now)) netlog::out.println("ok explore");
    else netlog::out.println("err: VL53L0X not found, will not drive blind");
    return;
  }
  if (!strcmp(cmd, "seek")) {
    if (n != 0) { netlog::out.println("err: usage: seek"); return; }
    stopMove();
    if (drivemode::set(drivemode::Mode::Seek, now)) netlog::out.println("ok seek");
    else netlog::out.println("err: need the VL53L0X and both BH1750 sensors (check wiring)");
    return;
  }
  if (!strcmp(cmd, "deepsleep")) {
    float min;
    if (n != 1 || !parseFloat(args[0], min) || min < 0.1f || min > 60) {
      netlog::out.println("err: usage: deepsleep <minutes 0.1-60>");
      return;
    }
    stopMove();
    drivemode::set(drivemode::Mode::Stop, now);
    netlog::out.printf("ok deepsleep %.1f min (offline until it reboots)\n", min);
    power::deepSleep((uint32_t)(min * 60000.0f));
  }
  if (!strcmp(cmd, "sleep")) {
    if (n != 0) { netlog::out.println("err: usage: sleep"); return; }
    stopMove();
    drivemode::set(drivemode::Mode::Sleep, now);
    netlog::out.println("ok sleep (send stop, explore or a move command to wake)");
    return;
  }
  // Any other command (manual driving etc.) takes over from explore/sleep.
  if (drivemode::current() != drivemode::Mode::Stop && strcmp(cmd, "?") && strcmp(cmd, "dist") &&
      strcmp(cmd, "buf") && strcmp(cmd, "arc") && strcmp(cmd, "i2c") && strcmp(cmd, "odo") &&
      strcmp(cmd, "lux") && strcmp(cmd, "cfg")) {
    drivemode::set(drivemode::Mode::Stop, now);
  }
  if (!strcmp(cmd, "i2c")) {
    power::sensorsWake();
    if (n != 0) { netlog::out.println("err: usage: i2c"); return; }
    int found = 0;
    for (uint8_t a = 0x08; a < 0x78; a++) {
      Wire.beginTransmission(a);
      if (Wire.endTransmission() == 0) {
        netlog::out.printf("i2c: found 0x%02X\n", a);
        found++;
      }
    }
    netlog::out.printf("i2c: %d device(s)\n", found);
    return;
  }
  if (!strcmp(cmd, "lux")) {
    power::sensorsWake();
    if (n != 0) { netlog::out.println("err: usage: lux"); return; }
    float l, r;
    if (!light::read(l, r)) { netlog::out.println("err: BH1750 not ready or read failed (try i2c)"); return; }
    netlog::out.printf("lux: left %.0f, right %.0f, diff %+.0f (left - right), avg %.0f\n", l, r, l - r, (l + r) / 2);
    return;
  }
  if (!strcmp(cmd, "cfg")) {
    if (n == 1 && !strcmp(args[0], "reset")) {
      ipsFull = INCHES_PER_SEC_FULL;
      distance::setBufferIn(DIST_BUFFER_IN);
      drivemode::setSeekArc(SEEK_ARC);
      prefs.remove("ips");
      prefs.remove("buf");
      prefs.remove("arc");
      netlog::out.println("ok cfg reset to config.h defaults");
    } else if (n != 0) {
      netlog::out.println("err: usage: cfg [reset]");
      return;
    }
    netlog::out.printf("cfg: cal %.2f in/s at 100%%, buf %.2f in, arc %.2f\n", ipsFull, distance::bufferIn(),
                       drivemode::seekArc());
    return;
  }
  if (!strcmp(cmd, "arc")) {
    float a;
    if (n == 1 && parseFloat(args[0], a) && a >= 0 && a <= 2) {
      drivemode::setSeekArc(a);
      prefs.putFloat("arc", a);
      netlog::out.printf("ok arc: %.2f (saved)\n", a);
    } else if (n != 0) {
      netlog::out.println("err: usage: arc [0-2]  (0 = no steering, 1 = inner side stops, 2 = pivot)");
      return;
    } else {
      netlog::out.printf("arc: %.2f\n", drivemode::seekArc());
    }
    return;
  }
  if (!strcmp(cmd, "buf")) {
    float in;
    if (n == 1 && parseFloat(args[0], in) && in >= 0 && in <= 20) {
      distance::setBufferIn(in);
      prefs.putFloat("buf", in);
      netlog::out.printf("ok buf: %.2f in (saved)\n", in);
    } else if (n != 0) {
      netlog::out.println("err: usage: buf [inches 0-20]");
      return;
    } else {
      netlog::out.printf("buf: %.2f in\n", distance::bufferIn());
    }
    return;
  }

  if (!strcmp(cmd, "dist")) {
    power::sensorsWake();
    if (n != 0) { netlog::out.println("err: usage: dist"); return; }
    if (!distance::ready()) { netlog::out.println("err: VL53L0X not found (check wiring)"); return; }
    int mm = distance::readMm();
    if (mm < 0) netlog::out.println("err: no reading (timeout or out of range)");
    else if (mm == distance::OUT_OF_RANGE_MM) netlog::out.println("dist: out of range");
    else netlog::out.printf("dist: %d mm (%.1f in), clearance %.1f in, buffer %.1f in\n", mm,
                            mm / 25.4f, distance::clearanceIn(mm), distance::bufferIn());
    return;
  }
  if (!strcmp(cmd, "odo")) {
    if (n == 1 && !strcmp(args[0], "reset")) {
      account(now);
      odoL = odoR = odoTotal = 0;
      saveOdo();
      netlog::out.println("ok odo reset");
    } else if (n != 0) {
      netlog::out.println("err: usage: odo [reset]");
      return;
    }
    printOdo();
    return;
  }
  if (!strcmp(cmd, "cal")) {
    long sp, ms;
    float in;
    if (n != 3 || !parseInt(args[0], sp) || !parseInt(args[1], ms) || !parseFloat(args[2], in) ||
        sp < 1 || sp > 100 || ms <= 0 || in <= 0) {
      netlog::out.println("err: usage: cal <speed 1-100> <ms> <inches>  (all > 0)");
      return;
    }
    ipsFull = in / (ms / 1000.0f) / (sp / 100.0f);
    prefs.putFloat("ips", ipsFull);
    netlog::out.printf("ok cal: %.2f in/s at 100%% (saved)\n", ipsFull);
    return;
  }
  if (!strcmp(cmd, "d")) {
    long sp;
    float in;
    if (n != 2 || !parseInt(args[0], sp) || !parseFloat(args[1], in) || sp < 1 || sp > 100 || in == 0) {
      netlog::out.println("err: usage: d <speed 1-100> <inches>  (negative inches = backward)");
      return;
    }
    float ips = ipsFull * sp / 100.0f;
    float ms = fabsf(in) / ips * 1000.0f;
    bool clamped = ms > MOVE_MAX_MS;
    if (clamped) ms = MOVE_MAX_MS;
    float v = (in > 0 ? sp : -sp) / 100.0f;
    startMove(now, v, v, (uint32_t)(ms + 0.5f));
    netlog::out.printf("ok L=%.0f%% R=%.0f%% for %.0f ms (~%.1f in%s)\n", v * 100, v * 100, ms,
                       (in > 0 ? 1 : -1) * ms / 1000.0f * ips, clamped ? ", clamped to max time" : "");
    return;
  }

  char c = cmd[0];
  bool known = cmd[1] == 0 && strchr("fblrm", c);
  if (!known) { netlog::out.printf("err: unknown command '%s' (? for help)\n", cmd); return; }

  int need = (c == 'm') ? 3 : 2;
  long a = 0, b = 0, ms = 0;
  bool ok = n == need && parseInt(args[0], a) && (c != 'm' || parseInt(args[1], b)) &&
            parseInt(args[need - 1], ms);
  if (!ok) {
    netlog::out.println(c == 'm' ? "err: usage: m <left> <right> <ms>" : "err: usage: <f|b|l|r> <speed> <ms>");
    return;
  }
  if (ms <= 0) { netlog::out.println("err: ms must be > 0"); return; }
  if (ms > (long)MOVE_MAX_MS) ms = MOVE_MAX_MS;

  if ((c == 'f' || c == 'b') && a < 0) {
    netlog::out.println("err: speed must be 0-100 for f/b (use m or l/r for signed)");
    return;
  }
  a = constrain(a, -100L, 100L);
  b = constrain(b, -100L, 100L);

  float l = 0, r = 0;
  switch (c) {
    case 'f': l = r = a / 100.0f; break;
    case 'b': l = r = -a / 100.0f; break;
    case 'l': l = a / 100.0f; break;
    case 'r': r = a / 100.0f; break;
    case 'm': l = a / 100.0f; r = b / 100.0f; break;
  }
  startMove(now, l, r, (uint32_t)ms);
  netlog::out.printf("ok L=%.0f%% R=%.0f%% for %ld ms\n", l * 100, r * 100, ms);
}

}  // namespace

bool isMoving() { return moving; }

void begin() {
  prefs.begin("rover", false);
  ipsFull = prefs.getFloat("ips", INCHES_PER_SEC_FULL);
  distance::setBufferIn(prefs.getFloat("buf", DIST_BUFFER_IN));
  drivemode::setSeekArc(prefs.getFloat("arc", SEEK_ARC));
  odoL = prefs.getFloat("odoL", 0);
  odoR = prefs.getFloat("odoR", 0);
  odoTotal = prefs.getFloat("odoT", 0);
  netlog::out.printf("[commands] buffer %.2f in\n", distance::bufferIn());
  netlog::out.printf("[commands] calibration %.2f in/s at 100%%\n", ipsFull);
}

void loop(uint32_t now) {
  bool connected = netlog::clientConnected();
  if (wasConnected && !connected && moving) {
    stopMove();
    netlog::out.println("[commands] client disconnected, motors stopped");
  }
  wasConnected = connected;

  if (moving && now - moveStart >= moveMs) {
    stopMove();
    netlog::out.println("done");
  }

  char line[96];
  while (netlog::readLine(line, sizeof(line))) handle(line, now);
}

}  // namespace commands
