# githooks

Tracked git hooks for the monorepo (`.git/hooks/` isn't versioned). Enable once per clone:

```sh
git config core.hooksPath githooks
```

- `commit-msg` — rejects commit subjects that don't match `<type>(<project|repo>): <summary>` (see [CLAUDE.md](../CLAUDE.md)). Scopes are discovered from top-level folders containing a `platformio.ini`, so new projects are accepted automatically.
- `pre-commit` — runs the word-clock Arduino sketch regeneration (`word-clock/githooks/pre-commit`) when word-clock sources are staged.
