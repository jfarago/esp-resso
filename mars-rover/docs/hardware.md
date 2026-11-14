# Hardware

## Parts

| Part | Qty | Link |
|---|---|---|
| Seeed XIAO ESP32-C6 | 1 | |
| _TBD_ | | |

## Specs

_TBD: per-part specs (supply voltage, current, interface, ratings)._

## Wiring

| Signal | Part / wire | ESP32-C6 pin |
|---|---|---|
| _TBD_ | | |

## Power

_TBD: supply, regulation, per-part voltages, common ground._

## Cautions

1. **C6 pin restrictions.** Avoid GPIO3 (RF switch power) and GPIO14 (antenna select), and the strapping pins GPIO4/5/8/9/15. See [xiao-esp32c6.md](../../docs/xiao-esp32c6.md).
2. **3.3 V logic.** ESP32-C6 GPIOs are not 5 V tolerant.
