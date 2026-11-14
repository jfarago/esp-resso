# Mars Rover

Firmware for a Mars rover on a Seeed XIAO ESP32-C6 (PlatformIO, Arduino framework via pioarduino). Hardware: `docs/hardware.md`. Board docs: `../docs/xiao-esp32c6.md`.

## Commands
- Wireless flash: `OTA_PASSWORD=<from secrets.h> pio run -e ota -t upload 2>&1 | tail -20` (needs one USB flash first)
- Build: `pio run 2>&1 | tail -20`
- Flash: `pio run -t upload 2>&1 | tail -20`
- Serial monitor: `pio device monitor 2>&1 | grep -v '^$' | tail -50` (or run with a timeout)
- Serial over Wi-Fi: `nc mars-rover.local 23` (all logs go through `netlog::out`, not `Serial`; replays the last 4 KB)
- Always pipe build/monitor output through `tail`/`grep`.

## Layout
`src/` firmware, `include/config.h` pins/battery constants, `include/secrets.h` (gitignored, copy from `secrets.example.h`: Wi-Fi + OTA password), `net.cpp` Wi-Fi/mDNS (`mars-rover.local`), `ota.cpp` ArduinoOTA, `docs/`. Battery monitoring is the shared `../lib/battery` library.

## Notes
- C6: avoid GPIO3 (RF switch power) and GPIO14 (antenna select); GPIO4/5/8/9/15 are strapping pins. The onboard user LED is GPIO15 (active-low).
- Never commit `include/secrets.h`.
