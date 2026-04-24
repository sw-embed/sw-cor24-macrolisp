# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

sw-cor24-macrolisp: Tiny Macro Lisp for COR24 — a minimal Lisp-1 with lexical scope, unhygienic defmacro, closures, and mark-sweep GC. Compiles to COR24 24-bit RISC assembly.

Forked from [sw-vibe-coding/tml24c](https://github.com/sw-vibe-coding/tml24c).

## Related Projects

- `~/github/sw-embed/sw-cor24-emulator` — COR24 assembler and emulator
- `~/github/sw-embed/sw-cor24-x-tinyc` — Tiny COR24 C compiler (Rust)

## Build

Uses [just](https://github.com/casey/just) as the command runner.

```bash
just build       # build test binary
just test        # run test suite
just run         # interactive REPL (Ctrl-] to exit)
just eval <file> # evaluate a .l24 file
just demo-blink  # LED blink demo
just clean       # clean build artifacts
```

Compiler flags: `-Wall -Wextra -Werror -std=c11`. Never suppress warnings.

## Changelog discipline

**Every commit updates `CHANGES.md`.** Before running `git commit`, add a one-line bullet to the current date's section (create the section if the date is new). Lead each bullet with the short commit SHA so the day reads as an ordered iteration log.

No exceptions for "trivial" demo tweaks, doc updates, or test-only commits — if it's shipping, it belongs in the changelog.

**If I'm about to commit without a CHANGES.md entry, stop and add one first.**

## Sibling consumers

`~/github/sw-embed/web-sw-cor24-macrolisp` embeds demo sources from `demos/*.l24` via `include_str!`. When a demo file changes here, the web UI needs to be rebuilt (`./scripts/build-pages.sh` over there) for the change to reach the live site — the web repo's `pages/` bundle is what GitHub Actions serves, and it captures these sources at build time.
