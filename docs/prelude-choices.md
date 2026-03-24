# Prelude Choices

## Overview

The tml24c prelude is compiled into the REPL binary as C strings evaluated at startup. Different use cases need different prelude sizes. This document proposes 4 prelude tiers.

## Prelude Tiers

### 1. Tiny (minimal)

**Use case**: Maximum heap/stack headroom, debugging, understanding core behavior.
**Stack**: Works with `--stack small` (3KB).
**Instruction cost**: ~500K to load.

Includes only what's needed for basic Lisp programming:

```
Primitives only (from C): +, -, *, /, %, <, =, cons, car, cdr, list,
  null?, pair?, atom?, eq?, not, print, println, newline, display,
  number?, string?, fn?, exit, peek, poke, delay, gc, heap-used,
  heap-size, number->string, apply, string-length, string-ref,
  string-append, string=?
```

No prelude definitions — just the C-registered primitives.

### 2. Standard (default)

**Use case**: General-purpose Lisp programming on COR24-TB.
**Stack**: Works with `--stack small` (3KB).
**Instruction cost**: ~3M to load.

Adds core Lisp functions:

```
From tiny, plus:
  map, filter, reduce, foldr, length, append, reverse, nth
  cadr, caddr, caar, cdar
  >, >=, <=, zero?, positive?, negative?, abs, min, max
  when, unless, let, cond, and, or
  identity, complement, compose
  for-each
  ->str, str2, str
  set!
  IO constants: IO-LED, IO-SWITCH, etc.
  set-leds, get-leds, s2-pressed?
```

### 3. Full (extended)

**Use case**: Clojure-style programming, demos, complex applications.
**Stack**: Requires `--stack large` (8KB).
**Instruction cost**: ~8M to load.

Adds everything in standard, plus:

```
From standard, plus:
  range, repeat, take, drop, zip, flatten
  range-down, interleave3
  every?, some, none?
  constantly, partial, juxt
  -> (thread-first), ->> (thread-last)
  doseq, dotimes
  lazy-cons, lazy-car, lazy-cdr, lazy?, lazy-take
  lazy-map, lazy-filter, iterate, lazy-range
  take-while, drop-while
  assoc, get
  trampoline, fn?
  aif, awhen, aand, acond (anaphoric macros)
  cond-expand (cond helper)
  true, false (aliases)
  for-each
```

### 4. Experimental

**Use case**: Testing new features, bleeding-edge development.
**Stack**: Requires `--stack large` (8KB).
**Instruction cost**: ~10M to load.

Everything in full, plus features that are expensive or not fully tested:

```
From full, plus:
  Multi-methods (defmulti, defmethod, invoke-multi)
  Memoize (make-memo)
  Atom/swap!/deref
  String formatting utilities
  Debug/trace primitives (when added)
```

## Implementation Approach

### Option A: Separate source files (simplest)

```
src/repl-tiny.c     → build/repl-tiny.s
src/repl.c          → build/repl.s        (standard, current)
src/repl-full.c     → build/repl-full.s
src/repl-exp.c      → build/repl-exp.s
```

Justfile recipes:
```
run:           # standard prelude
run-tiny:      # minimal
run-full:      # full prelude, --stack large
run-exp:       # experimental, --stack large
```

### Option B: Prelude as .l24 files (flexible)

```
prelude/tiny.l24
prelude/standard.l24
prelude/full.l24
prelude/experimental.l24
```

A bare REPL (`src/repl-bare.c`) with no prelude loads the selected `.l24` file via UART before interactive input:

```bash
cat prelude/full.l24 myapp.l24 | cor24-run --run build/repl-bare.s --terminal --stack large
```

**Pros**: No recompilation to switch preludes. Easy to customize.
**Cons**: Slower startup (prelude sent over UART), prelude uses instruction budget.

### Option C: Hybrid (recommended)

Standard prelude compiled in (option A). Additional features loaded from `.l24` files:

```bash
# Standard prelude + lazy sequences from file
cat prelude/lazy.l24 | cor24-run --run build/repl.s --terminal

# Bare REPL + full prelude from file
cat prelude/full.l24 | cor24-run --run build/repl-bare.s --terminal
```

## Migration Path

1. First: Factor current `load_prelude()` into tiers (split the C strings)
2. Second: Add `just run-tiny`, `just run-full` recipes
3. Third: Create prelude `.l24` files for option C flexibility
4. Fourth: Add `--stack` to cor24-run for full-prelude support
