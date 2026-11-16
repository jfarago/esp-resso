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
| `s` | `s` | Stop immediately |
| `?` | `?` | List commands and show motor state |

Every move is timed and stops itself. A new command replaces the current move. Durations are capped at 10 s (`MOVE_MAX_MS` in `include/config.h`), and the motors stop if the connection drops. Each command replies with `ok ...` or `err: ...`.

Distance is an open-loop estimate (power x time x calibration, assumed linear in power), so it drifts from real distance. The default calibration is 5.41 in/s at 100% power; recalibrate with `cal` after measuring a real run.

See `docs/hardware.md` and `../docs/xiao-esp32c6.md`.
