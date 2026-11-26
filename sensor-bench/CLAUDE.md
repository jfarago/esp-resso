# sensor-bench

Test bench for assorted sensors on a LilyGO T-Display S3 (ESP32-S3, 1.9" ST7789 170x320, touch version), PlatformIO + Arduino. First utility: INA228 current/power monitor (`src/main.cpp`).

## Pin map
| Pin | Function |
|---|---|
| GPIO15 | PIN_POWER_ON, must be HIGH for display/battery operation |
| GPIO38 | Backlight |
| GPIO14 | Front button (active low): hold 1.5 s to clear mAh. GPIO0 is the other button (BOOT, avoid) |
| GPIO43 / GPIO44 | I2C SDA / SCL to INA228 via the board's Qwiic/STEMMA QT connector (GPIO17/18 are the touch controller's own bus) |
| 5/6/7/8/9, 39-42, 45-48 | 8-bit parallel display bus (TFT_eSPI, set via `build_flags`) |

## Commands
- Build: `pio run 2>&1 | grep -E "error|SUCCESS|FAILED|RAM:|Flash:"`
- Flash: `pio run -t upload 2>&1 | tail -20`
- Serial monitor: `pio device monitor 2>&1 | tail -30` (prints a line per second; send `r` to clear mAh)

## Rules
- TFT_eSPI is configured only through `build_flags` in `platformio.ini` (`USER_SETUP_LOADED`); never edit the library's User_Setup.h.
- INA228 (Adafruit #5832, 0x40, 15 mOhm shunt, 10 A): the Adafruit library has no charge API and returns mV/mA/mW (`readBusVoltage` is mV). mAh comes from the chip's hardware CHARGE accumulator, read raw in `readMah()`; its LSB depends on `MAX_CURRENT_A` in `config.h`, which must match `setShunt()`.
- mAh and the elapsed timer are zeroed together in `resetMeasurement()` (boot, button hold, serial `r`); neither survives a reset. Always reset through it so they cover the same period.
- Pins, shunt and timings live in `include/config.h` (no secrets, tracked).
