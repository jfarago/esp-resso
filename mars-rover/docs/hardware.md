# Hardware

## Parts

| Part | Qty | Link |
|---|---|---|
| Seeed XIAO ESP32-C6 | 1 | |
| GA12-N20 6 V DC gear motor (60 or 100 RPM, no encoder) | 6 | 3 per side, wired in parallel |
| TB6612FNG dual H-bridge motor driver module | 1 | |
| VL53L0X time-of-flight distance sensor module | 1 | |
| BH1750 (GY-302) light sensor module | 2 | left and right |
| 3.7 V Li-ion / LiPo cell (protected) | 1 | |
| CN3065 solar LiPo charger module | 1 | |
| 6 V mini solar panel (1.5-2.5 W) | 1 | |
| DC-DC step-up boost module (MT3608 or fixed 5 V) | 1 | motor supply |
| 220 k resistor | 2 | battery sense divider |
| 100 k resistor | 1 | `STBY` pull-down |
| 100 nF ceramic capacitor | 7 | 1 per motor, 1 on the battery sense pin |
| 220-470 uF electrolytic capacitor (>= 10 V) | 1 | across `VM` / `GND` |
| SPDT power switch (>= 2 A) | 1 | |

See [project-overview.md](project-overview.md) for buying notes.

## Specs

| Part | Supply | Interface | Notes |
|---|---|---|---|
| XIAO ESP32-C6 | 5 V on `5V`/USB, 3.3 V logic (not 5 V tolerant) | | Wi-Fi, light sleep |
| GA12-N20 motors | 6 V nominal | PWM via TB6612FNG | no encoders, open-loop distance |
| TB6612FNG | `VM` 4.5-13.5 V, `VCC` 2.7-5.5 V | 3.3 V logic, PWM | 1.2 A continuous per channel |
| VL53L0X | 2.6-3.5 V (module `VIN` 3.3 V) | I2C 0x29 | about 2 m max range, narrow cone |
| BH1750 | 3.3 V | I2C 0x23 (left), 0x5C (right) | 1-65535 lux, 120 ms per reading |
| CN3065 | solar 4.4-6 V in | | 500 mA max charge, 4.2 V cutoff |
| Li-ion cell | 3.0-4.2 V | | measured through the 220k/220k divider |


## Wiring

| Signal | Part / wire | XIAO pin | GPIO |
|---|---|---|---|
| Battery sense | Divider midpoint (220k/220k) | MTCK pad (back) | 6 |
| `AIN1` | TB6612FNG, left motors | D6 | 16 |
| `AIN2` | TB6612FNG, left motors | D7 | 17 |
| `PWMA` | TB6612FNG, left motors | D8 | 19 |
| `BIN1` | TB6612FNG, right motors | D9 | 20 |
| `BIN2` | TB6612FNG, right motors | D10 | 18 |
| `PWMB` | TB6612FNG, right motors | D3 | 21 |
| `STBY` | TB6612FNG standby (not tied to 3V3) | D0 | 0 |
| `SDA` | Both BH1750 | D4 | 22 |
| `SCL` | Both BH1750 | D5 | 23 |
| `SDA` / `SCL` | VL53L0X (address 0x29), same bus | D4 / D5 | 22 / 23 |
| Peripheral power enable | HIGH = on, see below | D1 | 1 |
| Spare | | D2 | 2 (ADC-capable) |

- **STBY:** add a ~100 kΩ pull-down to GND so the driver stays disabled while the pin floats during boot/reset. Drive HIGH only after the pins are initialized. GPIO0 is an LP GPIO, so it can be held LOW through deep sleep with `gpio_hold_en()`.
- **Switched peripheral power:** D1 is LOW during idle light sleep (including `sleep` mode) and deep sleep. It goes HIGH at boot and again when a command or drive mode needs the sensors (`power::sensorsWake()`, which also re-initializes them); any new code that reads a sensor must call it first. It directly powers both BH1750s (including the right one's `ADDR`, which otherwise draws about 3 mA) and the VL53L0X. TB6612 `VCC` stays on 3V3. Wherever this page says those connect to 3V3, they connect to this switched supply. The VL53L0X can peak near 40 mA, which leaves little or no margin on the pin. If sensors drop out or reset, use an active-high load switch fed from 3V3, with D1 as its enable.
- **TB6612 power:** `VCC` to 3V3 (logic only), `VM` to the boost output (with the bulk cap), `GND` common.
- **BH1750 addresses:** share `VCC` (3V3), `GND`, `SDA`, `SCL`. Left sensor `ADDR` open/GND = 0x23; right sensor **`ADDR` to 3V3** = 0x5C (forgetting this puts both at 0x23, and seek will refuse to start; check with `i2c`). D4/D5 are the default `Wire` pins on the XIAO C6.
- **VL53L0X:** `VIN` to 3V3, `GND`, `SDA`/`SCL` on the shared bus; `XSHUT` and `GPIO1` left open. Default address 0x29 does not clash with the BH1750s. If a second VL53L0X is added, wire its `XSHUT` to D1/D2 and re-address at boot.
- **PWM:** only `PWMA`/`PWMB` need PWM; LEDC can route to any GPIO on the C6.

## Power

```
solar panel (6 V) -> CN3065 SOLAR IN
CN3065 BAT+/BAT- <-> Li-ion cell
cell BAT+ -> power switch -> boost module VIN   (motor current bypasses the CN3065 LOAD pin)
boost output (5.2-5.5 V) -> TB6612 VM (+ bulk cap) and XIAO 5V
XIAO 3V3 -> TB6612 VCC, BH1750 x2, VL53L0X
battery + -> 220k -> GPIO6 -> 220k -> GND  (100 nF from GPIO6 to GND)
```

- **Rails:** battery 3.0-4.2 V, motor rail 5.2-5.5 V, logic 3.3 V from the XIAO regulator.
- **Common ground:** battery -, CN3065, boost, TB6612, XIAO and all sensors share `GND`.
- **Brownouts:** the bulk cap across `VM`/`GND` keeps motor start-up from resetting the ESP32.

## Cautions

1. **C6 pin restrictions.** Avoid GPIO3 (RF switch power) and GPIO14 (antenna select), and the strapping pins GPIO4/5/8/9/15. See [xiao-esp32c6.md](../../docs/xiao-esp32c6.md).
2. **3.3 V logic.** ESP32-C6 GPIOs are not 5 V tolerant.
