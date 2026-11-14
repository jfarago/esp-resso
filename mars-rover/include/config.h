#pragma once
#include <stdint.h>

// Pins (XIAO ESP32-C6)
constexpr int PIN_VBAT = 6;  // battery divider midpoint, GPIO6 (MTCK pad on the back), ADC1_CH6

// Battery: BAT+ -> 220k -> PIN_VBAT -> 220k -> GND (ratio 2). A 100 nF cap from PIN_VBAT to GND
// steadies the ADC reading. Same wiring as ro-system-monitor; see ../lib/battery.
constexpr float VBAT_DIVIDER = 2.0f;
constexpr float VBAT_CAL = 1.0099f;         // copied from ro-system-monitor; re-tune against a multimeter on this board
constexpr int VBAT_SAMPLES = 16;
constexpr uint32_t VBAT_SAMPLE_MS = 1000;
constexpr float VBAT_SMOOTH_ALPHA = 0.2f;
constexpr uint32_t VBAT_LOG_MS = 60000;
constexpr float VBAT_PRESENT_MIN_V = 2.0f;
