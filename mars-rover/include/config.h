#pragma once
#include <stdint.h>

// Pins (XIAO ESP32-C6)
constexpr int PIN_VBAT = 6;  // battery divider midpoint, GPIO6 (MTCK pad on the back), ADC1_CH6

// Battery: BAT+ -> 220k -> PIN_VBAT -> 220k -> GND (ratio 2). A 100 nF cap from PIN_VBAT to GND
// steadies the ADC reading. Same wiring as ro-system-monitor; see ../lib/battery.
constexpr float VBAT_DIVIDER = 2.0f;
constexpr float VBAT_CAL = 1.0088f;         // tuned on this board: 4.062 V on a multimeter
constexpr int VBAT_SAMPLES = 64;
constexpr uint32_t VBAT_SAMPLE_MS = 1000;
constexpr float VBAT_SMOOTH_ALPHA = 0.2f;
constexpr uint32_t VBAT_LOG_MS = 60000;
constexpr float VBAT_PRESENT_MIN_V = 2.0f;

// Power modes (see docs/project-overview.md section 6). Hysteresis: Drive <-> Sun-seek uses 3.5/3.6 V.
constexpr float MODE_DRIVE_MIN_V = 3.6f;       // Sun-seek -> Drive at or above this
constexpr float MODE_SEEK_BELOW_V = 3.5f;      // Drive -> Sun-seek below this
constexpr float MODE_SLEEP_BELOW_V = 3.3f;     // placeholder: any mode -> Deep-sleep charge below this
constexpr float MODE_SLEEP_EXIT_V = 3.4f;      // placeholder: Deep-sleep -> Sun-seek at or above this

// Wi-Fi / OTA (credentials and OTA password live in secrets.h)
#define MDNS_HOSTNAME "mars-rover"
constexpr uint32_t WIFI_RETRY_MIN_MS = 10000;  // reconnect backoff 10 s, 20 s, 40 s, capped at 60 s
constexpr uint32_t WIFI_RETRY_MAX_MS = 60000;

// Wi-Fi log mirror: `nc mars-rover.local 23`
constexpr uint16_t LOG_TCP_PORT = 23;
constexpr size_t LOG_BACKLOG_BYTES = 32768;  // history replayed to a new client
