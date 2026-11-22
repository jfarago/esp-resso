#pragma once

// ---- Board (T-Display S3) ----
constexpr int PIN_POWER_ON = 15;  // must be HIGH to power display (and run on battery)
constexpr int PIN_BACKLIGHT = 38;
constexpr int PIN_BUTTON = 14;    // right-hand side button, active low (BOOT = GPIO0 is the other)

// ---- I2C to INA228 ----
// Not GPIO16/17/18/21: those are used by the touch controller on the touch version.
constexpr int PIN_SDA = 11;
constexpr int PIN_SCL = 10;
constexpr uint32_t I2C_FREQ = 400000;
constexpr uint8_t INA228_ADDR = 0x40;  // A0/A1 open

// ---- INA228 ----
// Adafruit #5832 ships with a 15 mOhm shunt, rated 10 A.
constexpr float SHUNT_OHMS = 0.015f;
constexpr float MAX_CURRENT_A = 10.0f;

// ---- UI ----
constexpr uint32_t CLEAR_HOLD_MS = 1500;      // hold button this long to zero mAh
constexpr uint32_t DISPLAY_PERIOD_MS = 100;
constexpr uint32_t SERIAL_PERIOD_MS = 1000;
