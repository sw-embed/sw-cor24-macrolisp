# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## CRITICAL: AgentRail Workflow (MUST follow)

This project uses AgentRail for structured handoffs between sessions. You MUST run these commands:

**FIRST thing every session** -- before reading code, before doing any work:
```bash
agentrail next
```
Read the output. It contains your plan, current step, prompt, skill docs, and past trajectories.

**LAST thing every session** -- after committing code, before ending:
```bash
agentrail complete --summary "what you accomplished" \
  --reward 1 \
  --actions "tools and approach used" \
  --next-slug <next-step> \
  --next-prompt "instructions for next agent"
```
Use `--reward -1 --failure-mode "reason"` if the step failed.
Use `--done` instead of `--next-*` if the saga is finished.

Do NOT skip these commands. The next agent session depends on your trajectory recording.

## Project

Tiny Macro Lisp (tml24c): a minimal Lisp-1 with lexical scope, unhygienic defmacro, closures, and mark-sweep GC. Compiles to COR24 24-bit RISC assembly.

## Related Projects

- `~/github/sw-embed/cor24-rs` -- COR24 assembler and emulator (Rust)
- `~/github/sw-vibe-coding/tc24r` -- Tiny COR24 compiler (Rust)
- `~/github/sw-vibe-coding/agentrail-domain-coding` -- Coding skills domain

## Available Task Types

`c-project-init`, `c-compile-fix`, `lisp-define-form`, `pre-commit`

## Build

```bash
make          # build
make clean    # clean
```

Compiler flags: `-Wall -Wextra -Werror -std=c11`. Never suppress warnings.
