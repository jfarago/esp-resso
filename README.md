# esp-resso

Monorepo for my ESP32 projects. Each project is a self-contained PlatformIO project.

| Project | Description |
|---|---|
| [ro-system-monitor](ro-system-monitor/) | RO system monitor |
| [mars-rover](mars-rover/) | Mars rover |
| [word-clock](word-clock/) | Word clock |

## Building

```sh
cd <project>
pio run
```

Each project keeps its own `platformio.ini`, `.gitignore`, and `include/secrets.h` (gitignored — create your own per project).

## Shared docs

[`docs/`](docs/) holds documentation shared across projects (e.g. [XIAO ESP32-C6 board notes](docs/xiao-esp32c6.md)). Project-specific hardware docs stay in each project's own `docs/`.

## Shared libraries

[`lib/`](lib/) holds PlatformIO libraries shared across projects. A project opts in with `lib_extra_dirs = ../lib` in its `platformio.ini`.

| Library | Description |
|---|---|
| [battery](lib/battery/) | LiPo voltage monitor: ADC + resistor divider, smoothing, presence detect. Configured with a `battery::Config` struct. Power-saving plan in [PLAN.md](lib/battery/PLAN.md). |
