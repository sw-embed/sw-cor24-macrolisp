# Fix REPL: Diagnosis and Resolution

## Symptom

`make run` filled the screen with repeated `> ` prompts scrolling by endlessly.
No input was ever read; no expressions were evaluated.

## Root Cause

Two independent bugs prevented the REPL from working:

### Bug 1: `getc_uart()` checked the wrong status bit (FIXED)

In `src/io.h`, `getc_uart()` polled bit `0x02` (CTS, always asserted) instead of bit `0x01` (RX data ready). This meant it never blocked — returning 0x00 immediately, filling `read_line()`'s buffer with nulls at full speed.

**Fix:** Changed `0x02` to `0x01` in `src/io.h`.

### Bug 2: Emulator fed UART input without flow control (FIXED)

The `cor24-run` CLI fed `--uart-input` bytes one per batch tick regardless of whether the program had consumed the previous byte. During startup (prelude loading), hundreds of input bytes were lost because nobody called `getc_uart()`.

**Fix:** Changed `run_with_timing()` and `run_step_mode()` in `cor24-rs/rust-to-cor24/src/run.rs` to check the UART status register (bit 0, RX ready) before feeding the next byte. This implements a FIFO-drain pattern — bytes are only delivered when the program has consumed the previous one.

### Bug 3: No test-free build for REPL/eval mode (FIXED)

The main binary runs all test suites before entering the REPL, consuming millions of instructions. For file evaluation via `--uart-input`, this wasted the entire instruction budget on tests.

**Fix:** Added `src/repl.c` — a separate entry point that initializes the system, loads the standard prelude, and enters the REPL directly. Built as `build/repl.s`, used by `make run` and `make eval`.

## UART Status Register Reference

| Bit | Mask | Meaning                              |
|-----|------|--------------------------------------|
| 0   | 0x01 | RX data ready (set when byte arrives)|
| 1   | 0x02 | CTS (always asserted in emulator)    |
| 2   | 0x04 | RX overflow                          |
| 7   | 0x80 | TX busy                              |

## Current State

- `make run` — launches REPL, correctly blocks waiting for UART input
- `make eval FILE=examples/demo.l24` — evaluates `.l24` files via UART and prints results
- `make test` — runs all 5 test suites (scaffold, reader, eval, gc, compile)

## Future: Interactive Terminal

True interactive REPL use requires `cor24-run` to gain a `--terminal` mode that:
- Sets the terminal to raw mode
- Reads stdin keystrokes and forwards them to UART RX
- Prints UART TX bytes directly to stdout (no `[UART TX]` decoration)
