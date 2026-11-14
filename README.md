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
