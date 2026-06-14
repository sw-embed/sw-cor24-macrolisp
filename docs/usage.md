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
just demo-reduce            # reduce/fold patterns (full prelude)
just demo-blink             # LED blink (--speed 500000)

# Build and test
just build                  # build test binary
just test                   # run all 5 test suites
just clean                  # remove build artifacts
```

## How It Works

tml24c is **not a native executable**. It compiles to COR24 assembly and runs on the COR24 emulator:

```
src/*.c  →  tc24r  →  build/*.s  →  cor24-asm  →  build/*.lgo  →  cor24-emu
```

The `just` recipes handle this pipeline. You don't run `./build/repl-standard.s` directly — it's assembly text, not a binary. `cor24-asm` assembles it to a loadable `.lgo`, which `cor24-emu` runs.

## Direct cor24-emu Invocation

If you need more control than `just` provides. The `just build-*` recipes already
produce the matching `.lgo` next to each `.s`, so you can run it straight away:

```bash
# Build first (produces build/repl-standard.s AND build/repl-standard.lgo)
just build-standard

# Interactive REPL
cor24-emu --lgo build/repl-standard.lgo --terminal --echo --speed 0

# Evaluate a file (piped stdin). --uart-file /dev/stdin loads the program into
# the UART RX buffer with flow control (and appends EOF), so no bytes are lost
# while the prelude is still loading. --quiet prints program output to stdout.
grep -v '^;;' myfile.l24 | cor24-emu --lgo build/repl-standard.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000

# With larger stack (for full prelude)
cor24-emu --lgo build/repl-full.lgo --terminal --echo --speed 0 --stack-kilobytes 8

# Timed execution (LED blink demo)
cor24-emu --lgo build/repl-standard.lgo --terminal --speed 500000

# Debug: dump CPU state after halt
cor24-emu --lgo build/repl-standard.lgo --terminal --speed 0 --dump

# Assemble by hand (if you need a raw .lgo from an arbitrary .s)
cor24-asm build/repl-standard.s -o build/repl-standard.lgo
```

## Available Binaries

| Binary | Prelude | Build | Stack |
|--------|---------|-------|-------|
| `build/repl-minimal.s` | 6 defs (comparison, predicates) | `just build-minimal` | 3 KB |
| `build/repl-standard.s` | ~50 defs (core Lisp) | `just build-standard` | 3 KB |
| `build/repl-full.s` | ~90 defs (+ lazy, threading, anaphora) | `just build-full` | 8 KB |
| `build/repl-bare.s` | none (for custom .l24 preludes) | `just build-bare` | 3 KB |
| `build/tml24c.s` | standard + test suite + compiler | `just build` | 3 KB |

## cor24-emu Flags

| Flag | Description |
|------|-------------|
| `--lgo FILE` | Load and run an assembled `.lgo` image |
| `--terminal` | Bridge stdin/stdout to UART (live interactive use) |
| `--uart-file FILE` | Load FILE into the UART RX buffer (flow-controlled, EOF appended) — use `/dev/stdin` for piped batch input |
| `--quiet` | Program (UART TX) output to stdout, emulator diagnostics to stderr |
| `--echo` | Echo typed characters (for interactive use) |
| `--speed 0` | Max speed (default: 100K IPS) |
| `--speed 500000` | 500K IPS (needed for calibrated `delay`) |
| `-n 500000000` | Instruction limit |
| `--stack-kilobytes 8` | Larger stack (default: 3) |
| `--dump` | Dump CPU/memory state on exit |
| `--trace N` | Dump last N instructions on exit |

## Why No -h/--help/--version

tml24c runs on a bare-metal 24-bit CPU with no operating system, no argc/argv, and no command-line parsing. The "binary" is COR24 assembly text loaded by the emulator. All user-facing flags belong to `cor24-emu`, not to tml24c itself.

To check versions:
```bash
cor24-emu --help          # emulator version and flags
cor24-asm --help          # assembler version and flags
tc24r --help              # compiler version (if available)
git log --oneline -1      # tml24c commit
```
