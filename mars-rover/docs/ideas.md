# Ideas and next steps

Where the project stands (as of the `seek` / `arc` commits), then what could come next. Roughly ordered by value for effort within each section.

## Where we are

**Working in firmware** (builds; wiring and behavior tuned on the bench only so far for motors, battery and distance):

- Wi-Fi, mDNS (`mars-rover.local`), OTA flashing, and a TCP log/command port (`nc mars-rover.local 23`) with a 32 KB log replay.
- Battery monitoring with calibration, smoothing, and a timestamped log line every 5 minutes.
- Idle light sleep (30 s with no `nc` client or motion), with a Wi-Fi wake window.
- TB6612FNG motor driver: timed moves (`f b l r m`), distance moves (`d`), calibration (`cal`), open-loop odometer (`odo`).
- VL53L0X distance sensor: `dist`, an adjustable obstacle buffer (`buf`), and a measured sensor offset.
- Drive modes: `explore` (wander, back up and turn at the buffer), `seek` (scan, align, steer toward light, park on plateau), `stop`, `sleep`.
- BH1750 pair for seek, adjustable steering (`arc`), `i2c` bus scan.
- Settings that survive reboot (flash): `cal`, `buf`, `arc`.

**Stubbed or not done:**

- The battery power modes in `main.cpp` are mostly stubs. Deep-sleep-charge only logs, and its thresholds (3.3 / 3.4 V) are placeholders.
- `docs/hardware.md` still has TBD sections: parts list, specs, and the power wiring.
- Nothing has been tested outdoors or in real sun. Seek has only been tuned by reasoning, not by driving.

## Quick wins

- [x] **`lux` command.** Print left/right lux (and the diff) on demand. Right now the only way to see what seek sees is to run it. This makes debugging sensor placement, `SEEK_BAND_FRAC`, and left/right swaps much easier.
- [x] **Log lux during `seek`.** A line every second or two with left, right, smoothed average, and best so far. Pairs well with the `lux` command and makes plateau stops explainable.
- [x] **`cfg` command.** Print every runtime setting at once (`cal`, `buf`, `arc`) and offer `cfg reset` to return to the `config.h` defaults.
- [x] **Proportional steering for seek.** Turn harder the more lopsided the light is, instead of the current on/off arc. Probably smoother and less wobbly than raising `arc`.

- [x] **Fill in `docs/hardware.md`.** Parts table, per-part specs, power section. Also mention the right BH1750 `ADDR` to 3V3 up front in the wiring table (it bit us once).

## Safety and reliability

- **`nc` port has no authentication.** Anyone on the Wi-Fi can drive the rover. OTA is password protected, the command port is not. Options: a shared token as a first line (`auth <token>`), or only accept drive commands from the first client.
- **Explore and seek keep running after `nc` disconnects.** Intentional for autonomy, but there is no cap. Consider a max run time, or stopping on low battery, so a forgotten `explore` can't run the battery flat.
- **Stuck detection.** With no encoders, a stuck rover keeps spinning wheels. Use "clearance didn't change after N seconds of driving forward" (or an IMU, see below) to trigger a backup-and-turn.
- **Blind reverse.** Backup is a fixed 0.6 s with nothing watching behind. A rear sensor, or a shorter reverse, would reduce the risk. Fine for now on an open floor.
- **Low-battery cutoff for motors.** Refuse to drive (or force seek / stop) below a voltage so a sagging battery doesn't brown out the ESP32 mid-move. Needs real measurements under motor load, since voltage drops when the motors start.
- **Single forward sensor.** The VL53L0X sees a narrow cone straight ahead, so it misses table edges, thin chair legs, and anything off to the side. Worth knowing before trusting explore around stairs or ledges.
- **Sensor failure policy.** Three failed reads stops the mode. Consider logging which read failed (distance vs light) to speed up diagnosis.

## Seek and charging

- **Measure charging, not just brightness.** A current/voltage sensor (INA219) on the solar input would let seek maximize actual charge power. Lux in a room can be high from a lamp that the panel barely uses. Alternatively read the CN3065 charge-status pins on a spare GPIO (D1/D2) to at least know when it's charging.
- **Re-seek over time.** After parking, the sun moves. Wake periodically (the 5 min timer already exists), compare lux to the parked value, and re-run seek if it dropped by some percentage.
- **Calibrate the scan time.** `SEEK_SCAN_MS` is a guess. A `spin` calibration command (like `cal`) that stores the time for a full turn would remove the guess. An IMU makes this exact (see below).
- **Better plateau logic.** Today it parks 10 s after the last new best. A rover that drives past a peak parks past it. Could back up to where the best was seen, or do a short local re-scan before parking.
- **Wire battery modes to behavior.** `Seek` currently only starts automatically if the rover was exploring. Decide the full policy: low battery while idle, deep-sleep-charge once parked and charging, wake and resume at the high threshold.
- **Deep sleep charge mode.** From the original plan: motor driver in standby, deep sleep, wake every few minutes to check the battery. Light sleep with Wi-Fi is far from free, and on a small panel the radio current may matter. Needs measurements.

## Navigation and behavior

- **Smarter exploring.** Wall following, a spiral or coverage pattern, or bias away from where it just turned. Random turns can get it stuck oscillating between two obstacles.
- **Choose turn direction by clearance.** Spin toward the side with more space instead of randomly. A few degrees of scan would tell it which.
- **Scripted routines over `nc`.** A small command sequence (`run f 50 1000; m 70 -70 400; ...`) or named routines.
- **Home and return.** Remember where the brightest spot was and drive back to it with the odometer, accepting the drift.

## Hardware additions (cheapest payoff first)

- **IMU (MPU6050 / BNO055).** Gives real headings for exact turns, spin calibration, stuck detection, tilt and rollover detection, and a better odometer. Shares the I2C bus.
- **Second distance sensor.** Angled left/right, or rear-facing. If both VL53L0X units share the bus, wire `XSHUT` to D1/D2 and re-address at boot (already noted in `hardware.md`).
- **Servo-mounted distance sensor.** Sweep to pick the clearest direction.
- **Solar current/voltage sensing** (INA219), as above.
- **Wheel encoders.** N20 motors with magnetic encoders replace open-loop distance with real distance.
- **Camera.** Would need a different board (XIAO ESP32-S3 Sense). Big step, probably a separate project.

## Telemetry and tooling

- **Web status page.** A small HTTP server on the rover showing battery, mode, lux, distance, and buttons for `explore`, `seek`, `stop`. Phone-friendly beats `nc`.
- **Battery history.** The log already has timestamped battery lines; a script to turn the `nc` replay into a chart (a discharge curve would help set the mode thresholds).
- **MQTT or Home Assistant.** Publish battery and mode, accept mode commands.
- **Wall-clock timestamps.** Log lines are uptime only. NTP over Wi-Fi when available would make long logs readable.
- **Scripted `nc` test runs.** A script that connects, sends a command sequence, and captures the log, for repeatable tuning runs.

## Code health

- **Split `commands.cpp`.** It's 300 lines and mixes parsing, motion, calibration, and every command. A table of command handlers (name, arg count, function) would shrink `handle()` and make new commands cheaper. Also removes the "commands that don't cancel a mode" exemption list.
- **Host-side tests for the state machines.** `drivemode` is pure logic around sensor reads and motor calls. Putting those behind small interfaces would allow unit tests for obstacle handling and plateau detection without hardware.
- **Share the log-line timestamp helper.** `drivemode`, `power`, and `main` each build `[HH:MM:SS]` prefixes by hand around `netlog::stamp`. A `netlog::logf(tag, ...)` would remove the repetition.
- **Shared I2C bus lib.** If more I2C parts arrive, a small bus module (init, scan, recovery from a hung bus) could live in `lib/`, though it should stay free of any project's `config.h`.
