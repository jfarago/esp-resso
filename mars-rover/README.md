# Mars Rover

ESP32-C6 (Seeed XIAO) Mars rover. Firmware drives a TB6612FNG motor driver, monitors the battery, and logs and accepts drive commands over Wi-Fi (`nc mars-rover.local 23`).

## Build & flash
```
pio run              # build
pio run -t upload    # flash over USB
pio device monitor   # serial at 115200
```

First flash over USB (needs `include/secrets.h`, copied from `secrets.example.h`). After that: `OTA_PASSWORD=<password> pio run -e ota -t upload` flashes over Wi-Fi to `mars-rover.local`.

## Remote control
Connect with `nc mars-rover.local 23`. The same connection streams the logs and accepts one-line commands. Speed is a percent (0-100), time is in milliseconds, distance is in inches.

| Command | Example | What it does |
|---|---|---|
| `f <speed> <ms>` | `f 80 1500` | Both sides forward |
| `b <speed> <ms>` | `b 50 2000` | Both sides backward |
| `l <speed> <ms>` | `l 60 500` | Left side only (negative speed = reverse), right stopped |
| `r <speed> <ms>` | `r -60 500` | Right side only (negative speed = reverse), left stopped |
| `m <left> <right> <ms>` | `m 70 -70 400` | Both sides, signed -100..100 (turns and pivots) |
| `d <speed> <inches>` | `d 50 20` | Drive a distance using the calibration (negative inches = backward) |
| `cal <speed> <ms> <inches>` | `cal 50 7400 20` | Calibrate from a measured run; saved to flash |
| `odo [reset]` | `odo` | Show or zero the estimated distance travelled |
| `dist` | `dist` | Read the VL53L0X: distance, clearance (reading minus `DIST_SENSOR_OFFSET_MM`) and buffer |
| `i2c` | `i2c` | Scan the I2C bus and list responding addresses (for wiring checks) |
| `buf [inches]` | `buf 1.5` | Show or set the clearance kept from obstacles (default 1 in, saved to flash) |
| `explore` | `explore` | Drive around autonomously: forward until within the buffer, back up, spin until clear, repeat |
| `seek` | `seek` | Find the brightest spot to recharge: spin to scan with the two BH1750s, face the brightest direction, drive toward the brighter side (avoiding obstacles), and stop when brightness stops improving. Also starts by itself if the battery drops below 3.5 V while exploring |
| `stop` (or `s`) | `stop` | Stop immediately and leave explore/sleep |
| `sleep` | `sleep` | Light sleep, even with `nc` connected; wakes on Wi-Fi traffic. Send `stop`, `explore` or a move to wake |
| `?` | `?` | List commands and show motor state |

Explore keeps driving if `nc` disconnects, so reconnect and send `stop`; it also stops itself if the sensor fails repeatedly or the rover is boxed in. Tuning (speeds, backup and turn times) is in `include/config.h`.

Every manual move is timed and stops itself. A new command replaces the current move. Durations are capped at 10 s (`MOVE_MAX_MS` in `include/config.h`), and the motors stop if the connection drops. Each command replies with `ok ...` or `err: ...`.

Distance is an open-loop estimate (power x time x calibration, assumed linear in power), so it drifts from real distance. The default calibration is 5.41 in/s at 100% power; recalibrate with `cal` after measuring a real run.

See `docs/hardware.md` and `../docs/xiao-esp32c6.md`.
