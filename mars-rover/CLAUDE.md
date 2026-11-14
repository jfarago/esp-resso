# Mars Rover

Firmware for a Mars rover on a Seeed XIAO ESP32-C6 (PlatformIO, Arduino framework via pioarduino). Hardware: `docs/hardware.md`. Board docs: `docs/xiao-esp32c6.md`.

## Commands
- Build: `pio run 2>&1 | tail -20`
- Flash: `pio run -t upload 2>&1 | tail -20`
- Serial monitor: `pio device monitor 2>&1 | grep -v '^$' | tail -50` (or run with a timeout)
- Always pipe build/monitor output through `tail`/`grep`.

## Layout
`src/` firmware, `include/` headers/config, `docs/`.

## Notes
- C6: avoid GPIO3 (RF switch power) and GPIO14 (antenna select); GPIO4/5/8/9/15 are strapping pins. The onboard user LED is GPIO15 (active-low).
- Never commit `include/secrets.h`.
