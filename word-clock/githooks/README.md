# githooks

Project-level hook for word-clock. In the esp-resso monorepo it is invoked
by the root `githooks/pre-commit`; enable hooks once per clone from the repo
root with `git config core.hooksPath githooks` (see `../../githooks/README.md`).

## What's here

- `pre-commit` — runs `scripts/generate_arduino_sketch.sh` and stages any
  resulting changes to `arduino/word_clock/`, so that folder can't drift out
  of sync with `src/`/`include/`.
