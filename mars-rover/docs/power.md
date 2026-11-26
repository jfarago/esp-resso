# Power draw and battery life

Battery: 1000 mAh. Draw is a current, in **mA**; battery capacity is in **mAh** (current x hours).

## Measured draw and estimated runtime

Measured on the rover, November 2026. Runtime is capacity divided by current, from a full battery with no solar input. The 80% column is an assumed allowance for capacity you can't use (cutoff voltage, conversion, cell age), not a measurement.

| State | Draw | Runtime at 1000 mAh | At 80% (800 mAh) |
|---|---|---|---|
| Deep sleep | 0.5 mA | 2000 h (about 83 days) | 1600 h (about 67 days) |
| Sleep mode (light sleep) | 12 mA average (2 mAh in 10 min) | 83 h (about 3.5 days) | 67 h (about 2.8 days) |
| Awake, not moving, `nc` connected | 42 mA average (7 mAh in 10 min) | about 24 h | about 19 h |
| Motors at 50% | about 250 mA | 4 h | 3.2 h |
| Motors at 100%, no load | about 500 mA | 2 h | 1.6 h |
| Motors at 100%, wheels under resistance | up to 700 mA | 1 h 26 min | 1 h 9 min |

- **Sleep mode** swings between 0.8 mA (asleep) and 40 mA (awake for network traffic). Use the average for estimates. A rough check: awake about 25% of the time at 40 mA (about 750 wakes per 5 min, 100 ms each) plus 0.8 mA otherwise gives about 11 mA. The average depends on how much traffic wakes the rover and on `POWER_WAKE_WINDOW_MS`.
- **Motor figures** are for continuous running. Manual moves are short and timed, so real use depends on how long the motors are actually on. Load at 50% has not been measured.
- **Not yet measured:** sensors in continuous use while stationary. It was not recorded whether the sensors were powered during the awake reading.

**Example day:** 10 minutes of motors at 50% (about 42 mAh) plus sleep mode the rest of the time (about 286 mAh) is about 328 mAh, or about 3 days on 1000 mAh. This leaves out time spent awake.

**Low-battery protection:** below 3.3 V the rover deep sleeps and wakes hourly to check the battery (resumes at 3.4 V). It still draws power while asleep, so this slows further discharge but does not stop it. How much capacity is left at 3.3 V has not been measured. The runtimes above do not allow for the cutoff beyond the 80% assumption.

## What has improved

| Change | Effect |
|---|---|
| Sensors (and the right BH1750 `ADDR` line) powered from D1, off in deep sleep | Deep sleep went from about 3.5 mA to 0.5 mA |
| Sensors also off in light sleep | Light sleep was about 4 mA before this; now 0.8 mA while asleep, 12 mA average including network wakes (different kinds of reading, so not a direct comparison) |
| Wi-Fi wake window | 100 ms. 40 ms caused repeated `BEACON_TIMEOUT` drops; occasional drops were still seen at 100 ms |

## Future improvements

- **Switch off the 5 V boost converter.** It only supplies the motor controller (the XIAO is connected directly to the battery), but it is always connected and is believed to be why deep sleep still draws 0.5 mA (its share has not been measured on its own). Add a switch so it can be cut off, either by hand or from a spare pin such as D2. For firmware control, use a load switch on the booster's input, or its enable pin if it has one and disabling it really stops the draw (on a step-up converter, the input can still pass through to the output). Size the switch for the motor current drawn from the battery (700 mA measured, with margin for start-up and stall), keep it off by default while the pin floats, and measure deep sleep again.
- **Fewer Wi-Fi wakes:** reduce broadcast/multicast traffic reaching the rover (about 750 wakes per 5 minutes were seen).
- **Tune the wake window:** try about 70 ms, watching for Wi-Fi drops over at least 30 minutes.

## Measurement log

Add a row after each change so results can be compared. Note where the meter was connected and for how long it averaged.

| Date | Firmware commit | State | Draw | Notes |
|---|---|---|---|---|
| 2026-11 | not recorded | Deep sleep | 0.5 mA | |
| 2026-11 | not recorded | Sleep mode | 12 mA average (0.8-40 mA) | 2 mAh over 10 min |
| 2026-11 | not recorded | Awake, not moving, `nc` connected | 42 mA average | 7 mAh over 10 min |
| 2026-11 | not recorded | Motors 50% | 250 mA | |
| 2026-11 | not recorded | Motors 100%, no load | 500 mA | |
| 2026-11 | not recorded | Motors 100%, resistance | up to 700 mA | |
