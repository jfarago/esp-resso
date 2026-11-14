# Battery Saving Plan

Goal: cut average current as far as possible while the dashboard stays reachable over Wi-Fi at any time. A 5–10 s delay on first page load is acceptable, so the Wi-Fi radio can sleep between beacons instead of listening all the time.

Branch: `battery-and-low-power-states`. Hardware: `docs/hardware.md`. Board: `../docs/xiao-esp32c6.md`.

## 1. Where the power goes today

Nothing in `src/` sets Wi-Fi power save, CPU frequency or sleep, and `loop()` never yields. The figures below are typical ESP32-C6 numbers, not measurements from this board. Phase 0 replaces them with real ones.

| Load | Approx. draw | Notes |
|---|---|---|
| Wi-Fi radio, always listening | 70–100 mA | Largest single item |
| CPU at 160 MHz, busy-looping | 15–25 mA | `loop()` has no delay |
| Flow sensors (2x) | up to 30 mA at 5 V | Always powered to count pulses; the floor unless the hardware changes |
| TDS module | 3–6 mA | Powered continuously |
| Battery divider (2x 220k) | ~10 µA | Negligible |

Note: the flow sensors are fed from the 5V pin (from USB or the boost), so their draw reflects on the battery through the boost converter. They are likely the largest remaining load once the firmware changes are in. Measure this first.

## 2. Design principle: web access through delayed wake, not "off"

The dashboard must be reachable at any time, so the radio cannot be switched off. Instead, keep the station associated and let it sleep between AP beacons:

- **Modem sleep with a long listen interval.** The radio wakes every N beacons (about 100 ms each) to check for buffered traffic. With a listen interval of 10 or so, a new connection waits up to about 1 s for the AP to deliver its packets. That fits the 5–10 s budget with room to spare.
- **Auto light sleep (Phase 4).** The CPU also sleeps between those wakes. This is where the large saving comes from, and it needs a build-time option (see Phase 4).

Consequences to accept:
- First page load and first `/api/status` poll are slow. After that, traffic keeps the radio awake for a short while and the UI feels normal.
- mDNS (`ro-monitor.local`) is multicast, which the AP only delivers on DTIM beacons. Resolution can be slow or flaky. Reserve a DHCP address on the router and bookmark the IP as a fallback.
- MQTT keepalive must be shorter than the broker's timeout, and a long listen interval can add jitter. Check the connection stays up.
- The dashboard's polling interval should be relaxed (for example 5 s instead of 1 s) and should pause when the tab is hidden.

## 3. Phases

Each phase is independently shippable. Measure after each one and record the result in the table in §6.

### Phase 0: Measure a baseline
- [ ] Put a current meter in series with the battery (INA219 board, USB power meter, or similar). Log average current over 10 min idle and 10 min with water flowing.
- [ ] Record: Wi-Fi connected and idle, Wi-Fi connected and dashboard open, flowing.
- [ ] Measure the flow sensors' share separately (unplug the 5 V sensor feed for one run, if it does not break the setup).
- [ ] Note the battery capacity and the target runtime without USB.

### Phase 1: Cheap firmware wins
- [ ] `setCpuFrequencyMhz(80)` in `setup()`. Confirm Wi-Fi, ADC readings and OTA still work.
- [ ] Add a small `delay()` (or `vTaskDelay`) at the end of `loop()` so the core idles. Counting is interrupt-driven, so this is safe. Keep the task watchdog fed.
- [ ] `WiFi.setSleep(WIFI_PS_MAX_MODEM)` after each STA connect. Set the listen interval with `esp_wifi_set_config` (`listen_interval`, start at 10). Re-apply after reconnect and after the radio reset in `net.cpp`.
- [ ] Lower TX power with `WiFi.setTxPower()` when RSSI is better than about -65 dBm. Re-evaluate on reconnect.
- [ ] Turn off the fallback AP as soon as the STA connects, and verify `WIFI_AP_STA` is only used while no network is available (check `net.cpp`).
- [ ] Add config constants in `include/config.h` for each knob so they can be tuned without code edits.

### Phase 2: Sample less
- [ ] Battery: sample every 30 s with 8 reads, not every 1 s with 16 (`VBAT_SAMPLE_MS`, `VBAT_SAMPLES`). Keep the smoothing, adjusting alpha so the time constant stays sensible.
- [ ] TDS: sample only while water is flowing and for a short time after (PLAN §4 already says a stagnant probe reads high). Hold the last good value otherwise. Add `TDS_ACTIVE_HOLD_MS`.
- [ ] MQTT: raise `MQTT_PUBLISH_MS` to 60–120 s; keep the change-triggered publishes.
- [ ] Serial: drop the periodic battery log to every few minutes, and skip the `while (!Serial)` wait when running from battery.
- [ ] UI: slow the status poll, and pause it on `visibilitychange`. Update `ui/` and `scripts/mock_server.py` together.

### Phase 3: Low-battery states
Use the smoothed battery voltage with hysteresis (about 0.1 V) so states do not flap. Voltages are starting points; adjust to the cell and its discharge curve.

| State | Entry | Behaviour |
|---|---|---|
| Normal | above 3.6 V | Everything as in Phases 1–2 |
| Saver | below 3.6 V | Longer MQTT interval, longer listen interval, UI poll hint raised, no fallback AP |
| Critical | below 3.4 V | Wi-Fi off (keep counting flow), save to NVS on every flow event end, check battery only |
| Cutoff | below 3.2 V | Save to NVS, then deep sleep with GPIO wake on the flow pins so counting resumes if power returns |

- [ ] Add the state machine in `battery.cpp` (or a new `power.cpp`), expose the state in `/api/status` and the MQTT state JSON (`ENTITIES` table in `src/ha_mqtt.cpp` must match).
- [ ] Show the state on the dashboard (battery chip colour or label).
- [ ] Recover from Critical to Saver when the voltage rises, for example while charging over USB.
- [ ] Hold `store::Lock` when saving from the state machine.

### Phase 4: Auto light sleep (biggest saving, needs a spike)
Goal: the CPU sleeps between Wi-Fi beacons and between flow pulses. This is what makes the long idle current small.

- [ ] Spike: the prebuilt Arduino core normally ships without power management enabled. Check whether pioarduino can build with a custom sdkconfig (`CONFIG_PM_ENABLE=y`, `CONFIG_FREERTOS_USE_TICKLESS_IDLE=y`, `CONFIG_PM_SLP_IRAM_OPT` if needed). I have not verified this; if it is not workable, stop here and keep Phases 1–3.
- [ ] Configure with `esp_pm_configure` (max 80 MHz, min lower, `light_sleep_enable = true`).
- [ ] Enable GPIO wake on both flow pins (`gpio_wakeup_enable`, `esp_sleep_enable_gpio_wakeup`). Verify that pulses are counted correctly at low and high flow, and that the 2 ms debounce still holds after a wake.
- [ ] Check ADC reads (TDS, battery), the task watchdog, OTA and MQTT keepalive under light sleep. OTA should keep the board awake (take a PM lock) for the transfer.
- [ ] Compare the counted gallons against a known 1 gal fill before and after, as in PLAN Phase 1.

### Phase 5: Hardware options (optional)
- [ ] Switch the TDS module's power from a free GPIO or a load switch so it is on only while measuring. Needs a spare pin that is not strapping, GPIO3 or GPIO14.
- [ ] Look at the flow sensors: a lower-current type, or powering them at the lowest rated voltage (3.3–5 V range depends on the part; check the datasheet in `docs/hardware.md`). Do not change the signal level without re-doing the §2.1 voltage check in PLAN.
- [ ] Charger and boost efficiency: if the 5 V rail comes from a boost converter, check its quiescent current and efficiency at light load.

## 4. Risks

- **Missed flow pulses.** Counting must stay exact. Every phase that touches CPU speed, sleep or loop timing is verified against a known-volume fill (Phase 4 especially).
- **Web access regressions.** Long listen intervals can make some routers drop the client. Test on the actual router, from a phone and a laptop.
- **mDNS reliability** under power save (see §2).
- **Watchdog.** `WDT_TIMEOUT_S` is 15 s. Any added delay or sleep must still let `loop()` come around well within that.
- **Cell protection.** The Cutoff state is a software guard only. Confirm the battery or charger has its own protection circuit.
- **Persistent storage.** More saves in the Critical state means more flash writes; keep them tied to flow-event ends.

## 5. Settings summary (proposed `config.h` additions)

| Constant | Start value | Purpose |
|---|---|---|
| `CPU_FREQ_MHZ` | 80 | CPU clock |
| `WIFI_LISTEN_INTERVAL` | 10 | Beacons between radio wakes |
| `WIFI_TX_POWER_LOW_RSSI` | -65 dBm | Threshold for lowering TX power |
| `VBAT_SAMPLE_MS` | 30000 | Battery sample interval |
| `VBAT_SAMPLES` | 8 | ADC reads per battery sample |
| `TDS_ACTIVE_HOLD_MS` | 60000 | TDS sampling after the last flow |
| `MQTT_PUBLISH_MS` | 60000–120000 | Periodic publish |
| `BATT_SAVER_V` / `BATT_CRITICAL_V` / `BATT_CUTOFF_V` | 3.6 / 3.4 / 3.2 | Low-battery thresholds |
| `BATT_HYSTERESIS_V` | 0.1 | State change hysteresis |

## 6. Results (fill in as phases land)

| Phase | Idle, Wi-Fi connected (mA) | Dashboard open (mA) | Flowing (mA) | First page load (s) |
|---|---|---|---|---|
| 0 baseline | | | | |
| 1 | | | | |
| 2 | | | | |
| 3 | | | | |
| 4 | | | | |

## 7. Open questions

- Battery capacity and the target runtime without USB.
- Is the 5 V rail for the flow sensors a boost converter from the cell, or USB only? This decides how much Phase 5 matters.
- Is the router's DHCP reservation possible, for a stable IP?
