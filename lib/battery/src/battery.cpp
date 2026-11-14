#include "battery.h"
#include <Arduino.h>

namespace battery {

static Config cfg;
static volatile float smoothV = -1.0f;  // may be read from another task, written from loop()
static uint32_t lastSampleMs = 0;
static uint32_t lastLogMs = 0;
static bool logged = false;

void begin(const Config& c) {
  cfg = c;
  smoothV = -1.0f;
  logged = false;
  pinMode(cfg.pin, INPUT);
  analogSetPinAttenuation(cfg.pin, ADC_11db);  // 0..~3.1 V at the pin; 4.2 V battery on a 1:2 divider reads 2.1 V
}

static float readVolts() {
  uint32_t mv = 0;
  for (int i = 0; i < cfg.samples; i++) mv += analogReadMilliVolts(cfg.pin);
  return (float)mv / cfg.samples / 1000.0f * cfg.divider * cfg.cal;
}

void update(uint32_t now) {
  bool first = smoothV < 0;
  if (!first && now - lastSampleMs < cfg.sampleMs) return;
  lastSampleMs = now;

  float v = readVolts();
  smoothV = first ? v : smoothV + cfg.smoothAlpha * (v - smoothV);

  if (cfg.logMs && (!logged || now - lastLogMs >= cfg.logMs)) {
    logged = true;
    lastLogMs = now;
    (cfg.out ? *cfg.out : (Print&)Serial).printf("[battery] %.3f V%s\n", (float)smoothV, present() ? "" : " (no battery detected)");
  }
}

float volts() { return smoothV; }

bool present() { return smoothV >= cfg.presentMinV; }

}  // namespace battery
