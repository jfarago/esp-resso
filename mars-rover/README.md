# Mars Rover

ESP32-C6 (Seeed XIAO) Mars rover. Currently a blink sketch to verify the toolchain.

## Build & flash
```
pio run              # build
pio run -t upload    # flash over USB
pio device monitor   # serial at 115200
```

First flash over USB (needs `include/secrets.h`, copied from `secrets.example.h`). After that: `OTA_PASSWORD=<password> pio run -e ota -t upload` flashes over Wi-Fi to `mars-rover.local`.

See `docs/hardware.md` and `../docs/xiao-esp32c6.md`.
