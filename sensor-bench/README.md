# sensor-bench

Test bench for sensors on a LilyGO T-Display S3 (ESP32-S3, 1.9" ST7789, 170x320).

## INA228 current monitor

Shows bus voltage (V), current (mA), power (mW) and cumulative charge (mAh) on the display, and prints the same once a second over USB serial. mAh is integrated in hardware by the INA228, so it doesn't miss samples. **Hold the front button (GPIO14) for 1.5 s** to clear it; a red bar shows hold progress. Sending `r` over serial also clears it.

### Wiring (Adafruit INA228, #5832)

| INA228 | T-Display S3 |
|---|---|
| VIN | 3V3 |
| GND | GND |
| SDA | GPIO11 |
| SCL | GPIO10 |
| VIN+ / VIN- | in series with the load, on the high or low side (max 85 V) |

The shunt is 15 mOhm / 10 A. For small loads (< ~2.7 A) call `ina.setADCRange(1)` after `setShunt()` for 4x finer resolution.

## Build

```sh
pio run
pio run -t upload
```
