# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## CRITICAL: AgentRail Session Protocol (MUST follow exactly)

This project uses AgentRail. Every session follows this exact sequence:

### 1. START (do this FIRST, before anything else)
```bash
agentrail next
```
Read the output carefully. It tells you your current step, prompt, skill docs, and past trajectories.

### 2. BEGIN (immediately after reading the next output)
```bash
agentrail begin
```

### 3. WORK (do what the step prompt says)
Do NOT ask the user "want me to proceed?" or "shall I start?". The step prompt IS your instruction. Execute it.

### 4. COMMIT (after the work is done)
Commit your code changes with git.

### 5. COMPLETE (LAST thing, after committing)
```bash
agentrail complete --summary "what you accomplished" \\
  --reward 1 \\
  --actions "tools and approach used"
```
If the step failed: `--reward -1 --failure-mode "what went wrong"`
If the saga is finished: add `--done`

### 6. STOP (after complete, DO NOT continue working)
Do NOT make any further code changes after running agentrail complete.
Any changes after complete are untracked and invisible to the next session.
If you see more work to do, it belongs in the NEXT step, not this session.

Do NOT skip any of these steps. The next session depends on your trajectory recording.

## Project

Tiny Macro Lisp (tml24c): a minimal Lisp-1 with lexical scope, unhygienic defmacro, closures, and mark-sweep GC. Compiles to COR24 24-bit RISC assembly.

## Related Projects

- `~/github/sw-embed/cor24-rs` -- COR24 assembler and emulator (Rust)
- `~/github/sw-vibe-coding/tc24r` -- Tiny COR24 compiler (Rust)
- `~/github/sw-vibe-coding/agentrail-domain-coding` -- Coding skills domain

## Available Task Types

`c-project-init`, `c-compile-fix`, `lisp-define-form`, `pre-commit`

## Build

Uses [just](https://github.com/casey/just) as the command runner.

```bash
just build       # build test binary
just build-repl  # build REPL binary
just test        # run test suite
just run         # interactive REPL (Ctrl-] to exit)
just eval <file> # evaluate a .l24 file
just demo-blink  # LED blink demo
just clean       # clean build artifacts
```

Compiler flags: `-Wall -Wextra -Werror -std=c11`. Never suppress warnings.
