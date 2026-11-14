#include <Arduino.h>
#include <battery.h>
#include "config.h"
#include "net.h"
#include "netlog.h"
#include "ota.h"

// XIAO ESP32-C6 user LED (GPIO15) is active-low; LED_BUILTIN comes from the board variant.
static const uint8_t LED_ON = LOW;
static const uint8_t LED_OFF = HIGH;

enum class Mode { Unknown, Drive, SunSeek, DeepSleepCharge };

static const char* modeName(Mode m) {
  switch (m) {
    case Mode::Drive: return "DRIVE";
    case Mode::SunSeek: return "SUN_SEEK";
    case Mode::DeepSleepCharge: return "DEEP_SLEEP_CHARGE";
    default: return "UNKNOWN";
  }
}

static Mode mode = Mode::Unknown;

// Pick the next mode from battery voltage, with hysteresis so it doesn't flap near a threshold.
static Mode nextMode(Mode cur, float v) {
  if (v < MODE_SLEEP_BELOW_V) return Mode::DeepSleepCharge;
  switch (cur) {
    case Mode::Drive:
      return v < MODE_SEEK_BELOW_V ? Mode::SunSeek : Mode::Drive;
    case Mode::SunSeek:
      return v >= MODE_DRIVE_MIN_V ? Mode::Drive : Mode::SunSeek;
    case Mode::DeepSleepCharge:
      if (v < MODE_SLEEP_EXIT_V) return Mode::DeepSleepCharge;
      return v >= MODE_DRIVE_MIN_V ? Mode::Drive : Mode::SunSeek;
    default:  // first reading
      if (v >= MODE_DRIVE_MIN_V) return Mode::Drive;
      return Mode::SunSeek;
  }
}

// Stub: real behavior comes with the hardware. For now just log what the mode would do.
static void onModeEnter(Mode m) {
  switch (m) {
    case Mode::Drive: netlog::out.println("[mode] would accept remote commands / explore"); break;
    case Mode::SunSeek: netlog::out.println("[mode] would steer toward the brighter light sensor"); break;
    case Mode::DeepSleepCharge: netlog::out.println("[mode] would stop motors and deep sleep, waking periodically to check battery"); break;
    default: break;
  }
}

// Uptime as HH:MM:SS (hours keep counting past 24). No wall clock: there is no NTP on this board.
static void logBattery(uint32_t now) {
  static uint32_t lastMs = 0;
  static bool logged = false;
  if (logged && now - lastMs < VBAT_LOG_MS) return;
  if (battery::volts() < 0) return;  // no sample yet
  logged = true;
  lastMs = now;
  uint32_t s = now / 1000;
  netlog::out.printf("[%02lu:%02lu:%02lu] [battery] %.2f V%s\n", (unsigned long)(s / 3600),
                     (unsigned long)(s / 60 % 60), (unsigned long)(s % 60), battery::volts(),
                     battery::present() ? "" : " (no battery detected)");
}

static void updateMode() {
  if (!battery::present()) return;
  float v = battery::volts();
  if (v < 0) return;
  Mode next = nextMode(mode, v);
  if (next == mode) return;
  netlog::out.printf("[mode] %s -> %s (%.2f V)\n", modeName(mode), modeName(next), v);
  mode = next;
  onModeEnter(mode);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  battery::Config bc;
  bc.pin = PIN_VBAT;
  bc.divider = VBAT_DIVIDER;
  bc.cal = VBAT_CAL;
  bc.samples = VBAT_SAMPLES;
  bc.sampleMs = VBAT_SAMPLE_MS;
  bc.smoothAlpha = VBAT_SMOOTH_ALPHA;
  bc.logMs = 0;  // main.cpp logs the battery itself (with a timestamp)
  bc.presentMinV = VBAT_PRESENT_MIN_V;
  battery::begin(bc);
  net::begin();
}

void loop() {
  uint32_t now = millis();
  net::loop(now);
  ota::loop();
  netlog::loop();
  battery::update(now);
  updateMode();
  logBattery(now);
  // Non-blocking 1 Hz blink so OTA stays responsive.
  digitalWrite(LED_BUILTIN, (now / 500) % 2 == 0 ? LED_ON : LED_OFF);
}
