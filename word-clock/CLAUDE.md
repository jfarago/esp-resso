# word-clock

WS2812 word clock on a Seeed XIAO ESP32-C3 (PlatformIO, Arduino framework, FastLED). Reads "IT IS [minutes] [PAST/TO] [hour] O'CLOCK" on an 8x15 letter grid. Overview, face diagram and setup: `README.md`.

## Commands
- Build: `pio run 2>&1 | tail -20`
- Flash: `pio run -t upload 2>&1 | tail -20`
- Serial monitor: `pio device monitor 2>&1 | grep -v '^$' | tail -50` (or run with a timeout)
- Regenerate Arduino IDE sketch: `./scripts/generate_arduino_sketch.sh`
- Always pipe build/monitor output through `tail`/`grep`.

## Layout
`src/` + `include/` firmware (PlatformIO source of truth), `include/config.h` WiFi/NTP/LED settings (gitignored, copy from `config.example.h`), `arduino/word_clock/` generated Arduino IDE copy, `assets/clock-face.svg` face diagram, `githooks/` project pre-commit hook.

| Module | Role |
|---|---|
| `main.cpp` | WiFi, NTP sync, render loop |
| `words.cpp` | time -> lit pixels; grid (row, col) -> serpentine strip index |
| `animations.cpp` | transitions: snap, crossfade, fade, wipe, ripple |
| `colorize.cpp` | solid / rainbow color modes |
| `boot_animation.cpp` | shown while waiting for WiFi/NTP |
| `settings.cpp` | settings persisted to NVS |
| `webui.cpp` | on-device HTTP UI at `http://wordclock.local/` |

## Rules
- **Never edit `arduino/word_clock/` by hand** (except its `README.md`). `word_clock.ino`, `words.*` and `config.example.h` there are generated from `src/`/`include/`; edit the sources and regenerate. The root pre-commit hook does this automatically when word-clock files are staged (enable with `git config core.hooksPath githooks` from the monorepo root).
- The LED strip is one serpentine run starting at the bottom-left row; always go through `wordGridStripIndex()` rather than computing indexes yourself.
- New user-facing settings: add a field to `Settings` in `include/settings.h`, persist it in `settings.cpp`, and expose it in `webui.cpp`.
- Call `webUiHandle()` frequently, including from inside long-running animations, so the UI stays responsive.
- Never commit `include/config.h` or `arduino/word_clock/config.h`.
