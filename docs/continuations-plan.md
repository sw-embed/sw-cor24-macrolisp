# Continuations in tml24c

## Architecture

The evaluator (`src/eval.h`) is a **trampoline loop** — a single `while(1)` that
reassigns `expr` and `env` and `continue`s for tail calls. Non-tail positions
(argument evaluation in `eval_list`, condition in `if`, intermediate `begin`
forms) use **recursive C calls**. The GC roots include the C call stack via
conservative scanning.

This means:
- The C stack is the implicit continuation — there is no reifiable frame object.
- Full `call/cc` (capturing and resuming arbitrary continuations) would require
  replacing the evaluator with a bytecode VM or CPS-transformed interpreter.

## What We Built: Escape Continuations

**Escape continuations** (one-shot, upward-only) are the high-value subset.
They let you:

- Bail out of deep recursion early (non-local exit)
- Build `try`/`catch`-style error handling
- Implement `guard` (R7RS) and `with-exception-handler`
- Write search/backtracking that aborts on first match
- Guarantee cleanup on any exit path (dynamic-wind, unwind-protect)

They do NOT let you:
- Resume a suspended computation (coroutines, generators)
- Re-enter a continuation multiple times (full `call/cc`)

## Implementation (bottom-up)

### Layer 1: catch/throw (C-level primitives)

`catch` is a special form; `throw` is a primitive. Both live in `src/eval.h`.

**Data structures:**
```c
#define MAX_CATCH_DEPTH 16

int catch_tags[MAX_CATCH_DEPTH];   // tag symbol for each catch frame
int catch_vals[MAX_CATCH_DEPTH];   // return value (filled by throw)
int catch_depth;                   // current stack depth
int catch_throwing;                // flag: unwinding in progress?
int catch_target;                  // depth to unwind to
```

**How it works:**
- `(catch tag-expr body-expr)` — eval tag, push catch frame, eval body. If body
  returns normally, pop frame, return body's value. If `catch_throwing` is set
  and `catch_target` matches this frame, clear flag, return `catch_vals[depth]`.
- `(throw tag value)` — search catch stack for matching tag. Run any pending
  `dynamic-wind` after thunks. Set `catch_throwing = 1`. Every recursive `eval`
  call checks `catch_throwing` after return and immediately returns `NIL_VAL`
  if set, unwinding the C stack back to the matching `catch` frame.

**Unwind checks** are inserted after every recursive `eval` call in:
`eval_list`, `if` condition, `define` value, `set!` value, `begin` intermediate
forms, function head eval, macro expansion, argument evaluation.

### Layer 2: dynamic-wind (C-level primitive)

```scheme
(dynamic-wind before-thunk body-thunk after-thunk)
```

**Data structures:**
```c
#define MAX_WIND_DEPTH 16

int wind_after[MAX_WIND_DEPTH];    // after thunk (closure) for each frame
int wind_depth;                    // current wind stack depth
```

**How it works** (in `PRIM_DYN_WIND`):
1. Call `before` thunk
2. Save `wind_depth`, push `after` to wind stack
3. Call `body` thunk via `apply_fn`
4. Restore `wind_depth` to saved value (throw may have already popped)
5. If not unwinding, call `after` thunk

**Throw integration:** `PRIM_THROW` walks the wind stack top-down and calls
each `after` thunk before setting `catch_throwing = 1`. This ensures cleanup
runs even on non-local exit.

**GC integration:** Both `wind_after[]` and `catch_tags[]/catch_vals[]` are
marked as roots in `gc_collect()` (`src/gc.h`).

**Key bug fixed:** Wind stack double-pop — when throw popped the wind stack
and then `PRIM_DYN_WIND` decremented again on return, `wind_depth` went
negative. Fix: save/restore instead of blind decrement.

### Layer 3: call/ec (prelude function)

```scheme
(define (call/ec proc)
  (let ((tag (gensym)))
    (catch tag
      (proc (lambda (val) (throw tag val))))))
```

Each `call/ec` invocation creates a unique gensym tag. The escape continuation
is a lambda that throws to that tag. Available in all preludes.

Scheme prelude also exports `call-with-escape-continuation` as an alias.

### Layer 4: raise / with-handler (prelude functions)

```scheme
(define *error-tag* (gensym))
(define *error-handler* nil)

(define (raise obj)
  (if (null? *error-handler*)
    (begin (display "ERROR: ") (println obj) (exit))
    (*error-handler* obj)))

(define (with-handler handler thunk)
  (let ((saved *error-handler*))
    (catch *error-tag*
      (begin
        (set! *error-handler*
          (lambda (e)
            (begin (set! *error-handler* saved)
                   (throw *error-tag* (handler e)))))
        (let ((result (thunk)))
          (begin (set! *error-handler* saved) result))))))

(define (error msg) (raise msg))
```

Handler chain is dynamic — `with-handler` saves and restores `*error-handler*`
on both normal return and error. Nested handlers work correctly: inner handler
catches first.

### Layer 5: guard (prelude macro)

```scheme
(defmacro guard (binding body)
  `(with-handler
    (lambda (,(car binding))
      ,(guard-clauses (car binding) (cdr binding)))
    (lambda () ,body)))
```

R7RS-inspired syntax: `(guard (e (test1 expr1) (test2 expr2) (else default)) body)`.
`guard-clauses` generates a chain of `if` tests against the error value.

### Layer 6: unwind-protect (prelude macro)

```scheme
(defmacro unwind-protect (body cleanup)
  `(dynamic-wind (lambda () nil) (lambda () ,body) (lambda () ,cleanup)))
```

Common Lisp style. Cleanup runs on both normal return and non-local exit.

### Layer 7: dynamic parameters (prelude)

```scheme
(define (make-parameter init)
  (let ((val init))
    (lambda args (if (null? args) val (set! val (car args))))))

(defmacro parameterize (bindings body)
  `(call-with-parameterize ,(caar bindings) ,(cadr (car bindings))
     (lambda () ,body)))
```

Parameters are closures over mutable cells. `parameterize` uses `dynamic-wind`
to save/restore the parameter value, ensuring restoration on non-local exit.

### Layer 8: restartable conditions (prelude)

```scheme
(define (with-restart name handler thunk)
  (let ((tag (gensym)))
    (let ((saved *restarts*))
      (begin
        (set! *restarts* (cons (list name tag handler) *restarts*))
        (let ((result (catch tag (let ((v (thunk)))
                        (begin (set! *restarts* saved) v)))))
          (begin (set! *restarts* saved) result))))))

(define (invoke-restart name val) ...)
```

CL-inspired: code establishes restarts, error handlers invoke them. The
handler runs in the signaler's dynamic context and can choose which restart
to invoke. Supports multiple active restarts and multi-error recovery
(e.g., map with skip-on-error).

## What This Enables

- **Error handling** without halting: `with-handler`, `guard`, `raise`
- **Early return** from loops: `call/ec` + `for-each`
- **Guard clauses** for input validation: `guard` with pattern matching
- **Resource cleanup**: `dynamic-wind`, `unwind-protect`
- **Short-circuit search**: `find-first`, `any?` via escape continuations
- **Configurable defaults**: `make-parameter`, `parameterize`
- **Recoverable errors**: `with-restart`, `invoke-restart`

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
GC root model).

## Possible Next Steps

- **Delimited continuations** (`shift`/`reset`) — less than full call/cc,
  enables generators and coroutines. Requires eval changes (stack capture)
  that the current trampoline architecture cannot support without a bytecode
  VM rewrite.
