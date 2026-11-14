# esp-resso

Monorepo of ESP32 projects. Each top-level folder is a self-contained PlatformIO project with its own `platformio.ini`, `.gitignore`, and (where present) its own `CLAUDE.md` — read the project's `CLAUDE.md` before working in it.

Projects: `ro-system-monitor`, `mars-rover`, `word-clock`.

## Shared code

`lib/` holds PlatformIO libraries shared across projects (currently `battery`). Projects use them via `lib_extra_dirs = ../lib` and pass their own pins/constants in at runtime; libraries must not include any project's `config.h`. Changes to `lib/` use scope `repo`, and verify every project that uses the library still builds.

## Commit messages

Use [Conventional Commits](https://www.conventionalcommits.org/) with the **project name as the scope**:

```
<type>(<project>): <short imperative summary>

<optional body: what and why>
```

- **type**: `feat`, `fix`, `docs`, `refactor`, `perf`, `test`, `build`, `chore`, `style`
- **scope**: the project folder name — `ro-system-monitor`, `mars-rover`, or `word-clock`
- Summary: imperative mood ("add", not "added"), lowercase, no trailing period, ~72 chars max
- Use scope `repo` for changes to root files (README, root `.gitignore`, this file, CI)
- Breaking changes: add `!` after the scope (`feat(word-clock)!: ...`) and a `BREAKING CHANGE:` footer

Examples:

```
feat(ro-system-monitor): add TDS sensor calibration endpoint
fix(mars-rover): correct motor direction on left track
docs(word-clock): document Arduino sketch generation
chore(repo): add root gitignore
```

### Rules

- One project per commit. If a change spans projects, split it into separate commits.
- A change that touches several projects only because of a shared repo-level concern uses `repo`.
- Never commit secrets: `include/secrets.h`, `config.h`, and `data/` are gitignored per project — only the `*.example.h` templates are tracked.
