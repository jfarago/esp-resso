#include <Arduino.h>
#include <battery.h>
#include "config.h"

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
    case Mode::Drive: Serial.println("[mode] would accept remote commands / explore"); break;
    case Mode::SunSeek: Serial.println("[mode] would steer toward the brighter light sensor"); break;
    case Mode::DeepSleepCharge: Serial.println("[mode] would stop motors and deep sleep, waking periodically to check battery"); break;
    default: break;
  }
}

static void updateMode() {
  if (!battery::present()) return;
  float v = battery::volts();
  if (v < 0) return;
  Mode next = nextMode(mode, v);
  if (next == mode) return;
  Serial.printf("[mode] %s -> %s (%.2f V)\n", modeName(mode), modeName(next), v);
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
  bc.logMs = VBAT_LOG_MS;
  bc.presentMinV = VBAT_PRESENT_MIN_V;
  battery::begin(bc);
}

void loop() {
  battery::update(millis());
  updateMode();
  digitalWrite(LED_BUILTIN, LED_ON);
  delay(500);
  digitalWrite(LED_BUILTIN, LED_OFF);
  delay(500);
}
