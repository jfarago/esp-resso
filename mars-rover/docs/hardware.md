# Hardware

## Parts

| Part | Qty | Link |
|---|---|---|
| Seeed XIAO ESP32-C6 | 1 | |
| _TBD_ | | |

## Specs

_TBD: per-part specs (supply voltage, current, interface, ratings)._

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
| Spare | | D1, D2 | 1, 2 (ADC-capable) |

- **STBY:** add a ~100 kΩ pull-down to GND so the driver stays disabled while the pin floats during boot/reset. Drive HIGH only after the pins are initialized. GPIO0 is an LP GPIO, so it can be held LOW through deep sleep with `gpio_hold_en()`.
- **TB6612 power:** `VCC` to 3V3 (logic only), `VM` to the boost output (with the bulk cap), `GND` common.
- **BH1750 addresses:** share `VCC` (3V3), `GND`, `SDA`, `SCL`. Left sensor `ADDR` open/GND = 0x23; right sensor `ADDR` to 3V3 = 0x5C. D4/D5 are the default `Wire` pins on the XIAO C6.
- **VL53L0X:** `VIN` to 3V3, `GND`, `SDA`/`SCL` on the shared bus; `XSHUT` and `GPIO1` left open. Default address 0x29 does not clash with the BH1750s. If a second VL53L0X is added, wire its `XSHUT` to D1/D2 and re-address at boot.
- **PWM:** only `PWMA`/`PWMB` need PWM; LEDC can route to any GPIO on the C6.

## Power

_TBD: supply, regulation, per-part voltages, common ground._

## Cautions

1. **C6 pin restrictions.** Avoid GPIO3 (RF switch power) and GPIO14 (antenna select), and the strapping pins GPIO4/5/8/9/15. See [xiao-esp32c6.md](../../docs/xiao-esp32c6.md).
2. **3.3 V logic.** ESP32-C6 GPIOs are not 5 V tolerant.
