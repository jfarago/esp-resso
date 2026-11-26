#pragma once
#include <stdint.h>

namespace power {

// Enables Wi-Fi modem power save. Call after net::begin().
void begin();

// Call every loop(). `busy` = something needs the CPU awake (motors moving, nc client connected,
// self-test running). Once idle for POWER_IDLE_MS, enters light sleep until Wi-Fi traffic or the
// periodic timer wakes it.
void loop(uint32_t now, bool busy);

// Forced light sleep (the `sleep` drive mode): sleeps even with an nc client connected, without waiting
// for POWER_IDLE_MS. Wi-Fi traffic still wakes it long enough to handle a command.
void setForced(bool forced);

// Deep sleep for `ms` (timer wake only), then the chip reboots. Wi-Fi is off meanwhile; does not return.
[[noreturn]] void deepSleep(uint32_t ms);

// Same, but touches nothing else (no Wi-Fi/log flush): for use before net::begin() at boot.
[[noreturn]] void deepSleepQuiet(uint32_t ms);

// Powers the sensors (PIN_PERIPH_PWR) and sets up I2C and both sensor drivers. Call before using any
// of them: at boot, and again later because light and deep sleep switch them off. Does nothing if they
// are already up; if a sensor failed to start, power-cycles and retries (unless the rover is driving).
void sensorsWake();

// True once the rover has been idle long enough that it is cycling through light sleep.
bool sleeping();

// Wi-Fi wakes since the last call; resets the count. Logged with the battery line instead of per wake.
uint32_t takeWifiWakes();

}  // namespace power
