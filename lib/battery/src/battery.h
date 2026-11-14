// LiPo battery voltage via a resistor divider on an ADC pin. Shared across projects.
#pragma once
#include <Print.h>
#include <stdint.h>

namespace battery {

struct Config {
  int pin;                         // ADC pin at the divider midpoint
  float divider = 2.0f;            // Vbat / Vpin
  float cal = 1.0f;                // multiplicative fudge from a multimeter comparison
  int samples = 16;                // ADC reads averaged per sample
  uint32_t sampleMs = 1000;        // time between samples
  float smoothAlpha = 0.2f;        // EMA factor (1 = no smoothing)
  uint32_t logMs = 60000;          // serial log interval; 0 disables logging
  float presentMinV = 2.0f;        // below this, treat as no battery connected
  Print* out = nullptr;            // log destination; nullptr = Serial
};

void begin(const Config& cfg);
void update(uint32_t nowMs);  // call every loop; samples every cfg.sampleMs
float volts();                // smoothed battery voltage; < 0 until the first sample
bool present();               // false if the divider reads below cfg.presentMinV

}  // namespace battery
