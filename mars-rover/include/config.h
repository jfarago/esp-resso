#pragma once
#include <stddef.h>
#include <stdint.h>

// Pins (XIAO ESP32-C6)
constexpr int PIN_VBAT = 6;  // battery divider midpoint, GPIO6 (MTCK pad on the back), ADC1_CH6

// Motor driver (TB6612FNG), see docs/hardware.md. Left = A channel, right = B channel.
constexpr int PIN_MOT_AIN1 = 16;  // D6
constexpr int PIN_MOT_AIN2 = 17;  // D7
constexpr int PIN_MOT_PWMA = 19;  // D8
constexpr int PIN_MOT_BIN1 = 20;  // D9
constexpr int PIN_MOT_BIN2 = 18;  // D10
constexpr int PIN_MOT_PWMB = 21;  // D3
constexpr int PIN_MOT_STBY = 0;   // D0, 100k pull-down to GND
constexpr uint32_t MOTOR_PWM_HZ = 20000;
// Left motors are mounted mirrored, so they must spin the opposite way for the rover to go forward.
constexpr bool MOTOR_LEFT_INVERT = true;
constexpr bool MOTOR_RIGHT_INVERT = false;

// Boot motor self-test: spin both sides forward after setup.
constexpr float MOTOR_TEST_POWER = 0.50f;  // fraction of full duty
constexpr uint32_t MOTOR_TEST_MS = 250;

// Remote drive commands over the log connection (see commands.cpp)
constexpr uint32_t MOVE_MAX_MS = 10000;  // longest single move; longer requests are clamped
// Open-loop distance estimate: ground speed at 100% power, assumed linear in power.
// Default from a measured run: 50% for 7400 ms = 20 in. Override at runtime with the `cal` command (saved to flash).
constexpr float INCHES_PER_SEC_FULL = 20.0f / 7.4f / 0.5f;

// Battery: BAT+ -> 220k -> PIN_VBAT -> 220k -> GND (ratio 2). A 100 nF cap from PIN_VBAT to GND
// steadies the ADC reading. Same wiring as ro-system-monitor; see ../lib/battery.
constexpr float VBAT_DIVIDER = 2.0f;
constexpr float VBAT_CAL = 1.0088f;         // tuned on this board: 4.062 V on a multimeter
constexpr int VBAT_SAMPLES = 64;
constexpr uint32_t VBAT_SAMPLE_MS = 1000;
constexpr float VBAT_SMOOTH_ALPHA = 0.2f;
constexpr uint32_t VBAT_LOG_MS = 300000;
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

// Light sleep (see power.cpp): after POWER_IDLE_MS with no nc client and no motion, the CPU sleeps and
// wakes on Wi-Fi traffic or every POWER_SLEEP_TIMER_MS (battery sample/log). After a Wi-Fi wake it
// stays up POWER_WAKE_WINDOW_MS so an incoming nc connection can be accepted.
constexpr uint32_t POWER_IDLE_MS = 30000;
constexpr uint32_t POWER_SLEEP_TIMER_MS = 300000;
constexpr uint32_t POWER_WAKE_WINDOW_MS = 100;
