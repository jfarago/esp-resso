#pragma once
#include <stddef.h>
#include <stdint.h>

// Pins (XIAO ESP32-C6)
constexpr int PIN_VBAT = 6;  // battery divider midpoint, GPIO6 (MTCK pad on the back), ADC1_CH6

// I2C bus (default Wire pins on the XIAO C6), shared by the BH1750 light sensors and the VL53L0X.
constexpr int PIN_I2C_SDA = 22;  // D4
constexpr int PIN_I2C_SCL = 23;  // D5
constexpr uint32_t I2C_HZ = 400000;
constexpr uint16_t DIST_TIMEOUT_MS = 500;  // VL53L0X read timeout

// Distance sensor placement: the VL53L0X reading when the rover's front is touching a wall (measured).
// Clearance = reading - offset. The buffer is how much clearance to keep from obstacles; this is only
// the default, change it at runtime with the `buf` command (saved to flash).
constexpr int DIST_SENSOR_OFFSET_MM = 58;
constexpr float DIST_BUFFER_IN = 1.0f;

// BH1750 light sensors on the shared I2C bus: left ADDR open/GND, right ADDR to 3V3 (docs/hardware.md).
constexpr uint8_t LIGHT_ADDR_LEFT = 0x23;
constexpr uint8_t LIGHT_ADDR_RIGHT = 0x5C;

// Seek mode (find the brightest spot to recharge, see drivemode.cpp). Spins to scan, aligns on the
// brightest direction, then drives toward the brighter side, stopping once brightness stops improving.
constexpr float SEEK_SPEED = 0.45f;             // forward power
constexpr float SEEK_ARC = 0.7f;                // default steering strength, set at runtime with `arc`: the inner side runs at (1 - arc) x speed (1 = stopped, 2 = full reverse pivot)
constexpr uint32_t SEEK_SCAN_MS = 4000;         // initial spin; ideally about one full turn, tune to your rover
constexpr float SEEK_ALIGN_FRAC = 0.9f;         // after the scan, keep spinning until lux >= this x scan best
constexpr float SEEK_BAND_FRAC = 0.08f;         // left/right difference ignored below this fraction of average lux
constexpr float SEEK_BAND_MIN_LUX = 20.0f;      // ...and never below this
constexpr float SEEK_SMOOTH_ALPHA = 0.3f;       // lux smoothing (1 = none)
constexpr float SEEK_IMPROVE = 1.03f;           // a new best must beat the old by this factor
constexpr uint32_t SEEK_PLATEAU_MS = 10000;     // no new best for this long = arrived, stop

// Explore mode (see drivemode.cpp)
constexpr float EXPLORE_SPEED = 0.50f;          // forward/backward power, fraction of full duty
constexpr float EXPLORE_TURN_SPEED = 0.60f;     // spin-in-place power
constexpr uint32_t EXPLORE_READ_MS = 60;        // distance read interval (each read blocks ~30 ms)
constexpr uint32_t EXPLORE_BACKUP_MS = 600;     // reverse after hitting the buffer, before turning
constexpr uint32_t EXPLORE_TURN_MIN_MS = 300;   // always turn at least this long
constexpr uint32_t EXPLORE_TURN_MAX_MS = 5000;  // give up (stop) if still blocked after this long turning
constexpr float EXPLORE_CLEAR_MARGIN_IN = 6.0f; // turn until clearance exceeds buffer + this

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
