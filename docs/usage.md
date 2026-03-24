# Usage

## Quick Reference

```bash
# Interactive REPL (Ctrl-] to exit)
just run                    # standard prelude
just run-minimal            # bare-bones (comparison ops only)
just run-full               # everything (lazy, threading, anaphora)

# Evaluate a .l24 file
just eval demos/strings.l24        # with standard prelude
just eval-full demos/lazy.l24      # with full prelude

# Custom prelude (slow, flexible)
just run-custom prelude/my-dialect.l24
just eval-custom myapp.l24 prelude/my-dialect.l24

# Run demos
just demo-bottles           # 99 Bottles of Beer
just demo-bottles2          # trampoline version
just demo-bottles4          # functional version
just demo-blink             # LED blink (--speed 500000)

# Build and test
just build                  # build test binary
just test                   # run all 5 test suites
just clean                  # remove build artifacts
```

## How It Works

tml24c is **not a native executable**. It compiles to COR24 assembly and runs on the COR24 emulator:

```
src/*.c  →  tc24r  →  build/*.s  →  cor24-run (emulator)
```

The `just` recipes handle this pipeline. You don't run `./build/repl-standard.s` directly — it's assembly text, not a binary.

## Direct cor24-run Invocation

If you need more control than `just` provides:

```bash
# Build first
just build-standard

# Interactive REPL
cor24-run --run build/repl-standard.s --terminal --echo --speed 0

# Evaluate a file (piped stdin)
grep -v '^;;' myfile.l24 | cor24-run --run build/repl-standard.s --terminal --speed 0 -n 500000000

# With larger stack (for full prelude)
cor24-run --run build/repl-full.s --terminal --echo --speed 0 --stack-kilobytes 8

# Timed execution (LED blink demo)
cor24-run --run build/repl-standard.s --terminal --speed 500000

# Debug: dump CPU state after halt
cor24-run --run build/repl-standard.s --terminal --speed 0 --dump
```

## Available Binaries

| Binary | Prelude | Build | Stack |
|--------|---------|-------|-------|
| `build/repl-minimal.s` | 6 defs (comparison, predicates) | `just build-minimal` | 3 KB |
| `build/repl-standard.s` | ~50 defs (core Lisp) | `just build-standard` | 3 KB |
| `build/repl-full.s` | ~90 defs (+ lazy, threading, anaphora) | `just build-full` | 8 KB |
| `build/repl-bare.s` | none (for custom .l24 preludes) | `just build-bare` | 3 KB |
| `build/tml24c.s` | standard + test suite + compiler | `just build` | 3 KB |

## cor24-run Flags

| Flag | Description |
|------|-------------|
| `--terminal` | Bridge stdin/stdout to UART |
| `--echo` | Echo typed characters (for interactive use) |
| `--speed 0` | Max speed (default: 100K IPS) |
| `--speed 500000` | 500K IPS (needed for calibrated `delay`) |
| `-n 500000000` | Instruction limit |
| `--stack-kilobytes 8` | Larger stack (default: 3) |
| `--dump` | Dump CPU/memory state on exit |
| `--trace N` | Dump last N instructions on exit |

## Why No -h/--help/--version

tml24c runs on a bare-metal 24-bit CPU with no operating system, no argc/argv, and no command-line parsing. The "binary" is COR24 assembly text loaded by the emulator. All user-facing flags belong to `cor24-run`, not to tml24c itself.

To check versions:
```bash
cor24-run --help          # emulator version and flags
tc24r --help              # compiler version (if available)
git log --oneline -1      # tml24c commit
```
