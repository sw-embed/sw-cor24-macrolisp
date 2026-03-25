# Continuations Plan for tml24c

## Current Architecture

The evaluator (`src/eval.h`) is a **trampoline loop** — a single `while(1)` that
reassigns `expr` and `env` and `continue`s for tail calls. Non-tail positions
(argument evaluation in `eval_list`, condition in `if`, intermediate `begin`
forms) use **recursive C calls**. The GC roots include the C call stack via
conservative scanning.

This means:
- The C stack is the implicit continuation — there is no reifiable frame object.
- Full `call/cc` (capturing and resuming arbitrary continuations) would require
  replacing the evaluator with a bytecode VM or CPS-transformed interpreter.
- That is a multi-week rewrite and out of scope for now.

## What We *Can* Build: Escape Continuations

**Escape continuations** (one-shot, upward-only) are the high-value subset.
They let you:

- Bail out of deep recursion early (non-local exit)
- Build `try`/`catch`-style error handling
- Implement `guard` (R7RS) and `with-exception-handler`
- Write search/backtracking that aborts on first match

They do NOT let you:
- Resume a suspended computation (coroutines, generators)
- Re-enter a continuation multiple times (full `call/cc`)

### How escape continuations work

```scheme
(call/ec (lambda (return)
  (for-each (lambda (x)
    (if (= x 3) (return x)))  ;; non-local exit
    '(1 2 3 4 5))
  'not-found))
;; => 3
```

`call/ec` captures the current "return point". Invoking the escape continuation
jumps back to that point with a value. The continuation becomes invalid once
`call/ec`'s body returns normally.

## Implementation Plan

### Step 1: catch/throw primitives (C-level)

Add a **tag stack** to the evaluator:

```c
#define MAX_CATCH_DEPTH 16

int catch_tags[MAX_CATCH_DEPTH];    // tag symbol for each catch frame
int catch_vals[MAX_CATCH_DEPTH];    // return value (filled by throw)
int catch_depth;                    // current depth
int catch_throwing;                 // flag: are we unwinding?
int catch_target;                   // depth to unwind to
```

- `(catch 'tag body)` — push a catch frame, eval body. If body returns
  normally, pop frame and return body's value. If a throw unwinds to this
  frame, return the thrown value.
- `(throw 'tag value)` — set `catch_throwing = 1`, `catch_target` to the
  matching frame depth, `catch_vals[depth] = value`. The eval loop checks
  `catch_throwing` after every sub-eval and returns immediately if set,
  unwinding the C stack back to the matching `catch` frame.

**Test:** `(catch 'done (begin (throw 'done 42) 99))` => `42`

### Step 2: call/ec (escape continuations as closures)

Build `call/ec` on top of catch/throw using gensym tags:

```scheme
(define (call/ec proc)
  (let ((tag (gensym)))
    (catch tag
      (proc (lambda (val) (throw tag val))))))
```

This can be a macro or a prelude function. The escape continuation is a
lambda that throws to the unique tag.

**Test:** early-return from `for-each`, nested `call/ec`, expired continuation
detection.

### Step 3: Error handling (guard/raise)

Build R7RS-style error handling on top of catch/throw:

```scheme
(define *current-handler* #f)

(define (raise obj)
  (if *current-handler*
    (*current-handler* obj)
    (begin (display "unhandled error: ") (println obj) (exit))))

(defmacro guard (var clauses body)
  ...)
```

Or simpler: `with-handler` / `raise` pattern.

**Test:** divide-by-zero guard, nested handlers, re-raise.

### Step 4: Demo — search with early exit

A practical demo showing escape continuations used for:
1. Early return from list search
2. Error recovery in arithmetic
3. Nested handler patterns

## What This Enables (post-implementation)

- **Error handling** without halting the program
- **Early return** from loops and deep recursion
- **Guard clauses** for input validation
- **Resource cleanup** patterns (dynamic-wind could follow)

## What Full call/cc Would Require (future)

To go beyond escape continuations to full first-class continuations:

1. **Bytecode VM**: Replace the trampoline eval loop with an opcode
   interpreter that maintains an explicit frame stack in heap-allocated
   arrays.
2. **Continuation objects**: Capture the frame stack as a heap object
   (`ETYPE_CONTINUATION`). Invoking it restores the saved stack.
3. **GC integration**: Continuation objects must be traced by the GC.
   The 256-entry root stack may need expansion.
4. **Memory budget**: Continuation captures are O(stack depth) in heap
   cells. With 32K cells total, this limits practical depth.

This is a significant architectural change (new opcode set, compiler pass,
GC root model) — estimated at 2-4 weeks of work.
