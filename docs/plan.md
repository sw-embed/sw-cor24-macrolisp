# Implementation Plan: Tiny Macro Lisp (tml24c)

## Overview

Six phases, each building on the last. Host-first development — all phases run on host (gcc/clang) before COR24 porting.

---

## Phase 0: Project Scaffold

**Goal:** Buildable C project with Makefile, header, and platform abstraction.

**Steps:**
1. Create `src/tml.h` — Value typedef, tag constants, forward declarations
2. Create `src/platform_host.c` — putchar/getchar/halt via stdio
3. Create `src/main.c` — skeleton main with init and test stub
4. Create/update `Makefile` — compile all .c in src/, link, `-Wall -Wextra -Werror -std=c11`
5. Verify: `just clean && just build` succeeds with zero warnings

**Deliverable:** Empty program that compiles and runs, prints "tml24c ok".

**AgentRail step:** `003-scaffold` / task type: `c-project-init`

---

## Phase 1: Core Evaluator (No GC, No Macros)

### Phase 1a: Heap + Symbols + Cons

**Goal:** Allocate cons cells and intern symbols. Bump allocator (no free/GC yet).

**Steps:**
1. Implement `heap.c` — cell array, `alloc_cell()` bump allocator, `alloc_cells(n)` for multi-cell objects
2. Implement `symbol.c` — `intern(name)` returns symbol Value; linear search of symbol list
3. Add fixnum/cons/symbol constructors and accessors to `tml.h`
4. Test: construct `(a b c)` in C, verify tag checks

### Phase 1b: Reader

**Goal:** Parse text into Value trees.

**Steps:**
1. Implement `read.c` — `read_expr()` dispatching on first character
2. Handle: whitespace, `;` comments, `(` lists `)`, `'` quote shorthand
3. Handle: integer literals, symbol names
4. Test from C: `read_expr("(+ 1 2)")` produces correct cons structure

**AgentRail step:** `005-reader` / task type: `lisp-define-form`

### Phase 1c: Printer

**Goal:** Print Values as text.

**Steps:**
1. Implement `print.c` — `print_val(v)` dispatching on tag
2. Handle: fixnums, symbols, proper lists, nil
3. Test: print what reader produced, verify round-trip

### Phase 1d: Environment + Evaluator

**Goal:** Evaluate expressions with global bindings.

**Steps:**
1. Implement `env.c` — `env_lookup`, `env_define`, `env_set`, `env_extend`
2. Implement `eval.c` — `eval(expr, env)` with:
   - Self-evaluating fixnums
   - Symbol lookup
   - Special forms: `quote`, `if`, `define`, `lambda`, `do`
   - Function application (closure + primitive dispatch)
3. Implement `builtin.c` — register primitives: cons, car, cdr, eq?, atom?, null?, +, -, *, <, print
4. Implement closure creation and application
5. Wire up REPL in `main.c`: read → eval → print loop

**Tests (entered at REPL or eval'd from C strings):**
```lisp
42                          ;=> 42
(+ 1 2)                    ;=> 3
(define x 10)              ;=> x
x                          ;=> 10
(if (< 1 2) 'yes 'no)     ;=> yes
(define id (lambda (x) x))
(id 42)                    ;=> 42
(define fact (lambda (n) (if (< n 2) 1 (* n (fact (- n 1))))))
(fact 5)                   ;=> 120
```

**AgentRail step:** `006-eval-core` / task type: `lisp-define-form`

### Phase 1e: Lexical Closures

**Goal:** Closures capture their defining environment.

**Test:**
```lisp
(define make-adder (lambda (n) (lambda (x) (+ x n))))
(define add2 (make-adder 2))
(add2 5)                   ;=> 7
```

This should work naturally if lambda captures the current environment.

---

## Phase 2: Garbage Collector

**Goal:** Mark-sweep GC so programs can allocate indefinitely.

**Steps:**
1. Add mark bitmap to `heap.c`
2. Implement `gc.c` — `gc_mark(val)` recursive marker, `gc_sweep()` free-list builder
3. Add temp-root stack (`PUSH_ROOT`/`POP_ROOT`) to protect live values during allocation
4. Instrument `alloc_cell()`: if free list empty, trigger GC; if still empty, error
5. Add GC to reader and eval (protect intermediate values)
6. Stress test: loop creating and discarding lists

**Test:**
```lisp
(define stress (lambda (n)
  (if (< n 1) 'done
    (do (list 1 2 3 4 5 6 7 8 9 10)
        (stress (- n 1))))))
(stress 10000)             ;=> done (with GC running many times)
```

**AgentRail step:** `007-gc` / task type: `c-compile-fix` (likely involves fixing GC root bugs)

---

## Phase 3: Macros

**Goal:** `defmacro`, quasiquote, gensym. Bootstrap library in Lisp.

### Phase 3a: defmacro + Quasiquote

**Steps:**
1. Add macro object type (like closure but tagged ETYPE_MACRO)
2. Add `defmacro` special form to eval
3. Add macro detection in eval: if operator is macro, apply to unevaluated args, eval result
4. Implement quasiquote processing in reader (syntax) and eval (expansion)
5. Implement `gensym`

**Test:**
```lisp
(defmacro when (test . body)
  `(if ,test (do ,@body) nil))
(when (< 1 2) (print 'hello) 'yes)  ;=> yes (prints hello)
```

### Phase 3b: Bootstrap Library

**Steps:**
1. Create `lib/stdlib.lisp` with:
   - `not`, `when`, `unless`, `cond`, `and`, `or`
   - `caar`, `cadr`, `cdar`, `cddr`
   - `list` (if not already builtin)
   - `append`, `reverse`
   - `map`, `filter`, `foldl`
   - `length`
2. Create `aif`, `awhen` anaphoric macros
3. Load stdlib at startup (embed as C string or `load` from file)

**Test:**
```lisp
(map (lambda (x) (* x x)) '(1 2 3 4 5))  ;=> (1 4 9 16 25)

(aif (+ 1 2) it nil)                       ;=> 3

(define items '((name . alice) (age . 30) (name . bob)))
(aif (filter (lambda (p) (eq? (car p) 'name)) items)
  (map cdr it)
  'none)                                    ;=> (alice bob)
```

---

## Phase 4: Strings

**Goal:** String type for practical programs and better error messages.

**Steps:**
1. Add string object type to heap (length + char buffer)
2. Add string literal parsing to reader (`"hello\n"` with escapes)
3. Add string printing (with and without quotes)
4. Add string builtins: `string-length`, `string-ref`, `string-append`, `string=?`
5. Add `number->string`, `symbol->string`

**Test:**
```lisp
(define greeting "Hello, COR24!")
(print greeting)                           ;=> Hello, COR24!
(string-length greeting)                   ;=> 13
(string-append "Hi " "there")             ;=> "Hi there"
```

---

## Phase 5: COR24 Target

**Goal:** Compile with tc24r, run on cor24-rs emulator.

### Phase 5a: Single-File Build

**Steps:**
1. Create `src/platform_cor24.c` — UART I/O via MMIO (0xFF0100), halt via self-branch
2. Create `tml24c_all.c` that `#include`s all source files (tc24r requires single-file)
3. Compile: `tc24r tml24c_all.c -o tml24c.s`
4. Fix any tc24r incompatibilities (no varargs, no bitfields, etc.)
5. Assemble and load in emulator

### Phase 5b: Emulator Testing

**Steps:**
1. Run REPL in emulator, test basic expressions via UART
2. Embed test expressions as startup strings (no file I/O on COR24)
3. Verify GC works under COR24 memory constraints
4. Tune heap size for available SRAM

### Phase 5c: Board Demo (stretch goal)

**Steps:**
1. Load LGO via UART to COR24-TB board
2. Run REPL over UART terminal
3. Demo: LED control from Lisp, anaphoric macro demo

**AgentRail step:** `008-compiler` / `009-integration`

---

## Phase 6: Polish

**Goal:** Documentation, cleanup, final demos.

**Steps:**
1. Complete test suite
2. Document all special forms and builtins
3. Write demo programs showcasing macros, closures, list processing
4. Ensure `just clean && just build` is zero-warning on host
5. Verify emulator demo end-to-end

---

## Milestone Summary

| Phase | Milestone | Key Test |
|-------|-----------|----------|
| 0 | Scaffold compiles | `just build` succeeds |
| 1a | Heap + symbols | Construct `(a b c)` in C |
| 1b | Reader | Parse `(+ 1 2)` from text |
| 1c | Printer | Print `(+ 1 2)` back |
| 1d | Evaluator | `(fact 5)` → 120 |
| 1e | Closures | `(make-adder 2)` works |
| 2 | GC | 10K iterations without OOM |
| 3a | Macros | `(when t 'yes)` → yes |
| 3b | Stdlib | `(map square '(1 2 3))` works |
| 4 | Strings | `"hello"` works |
| 5 | COR24 | REPL on emulator |
| 6 | Done | Full demo suite passes |

## Estimated Complexity

| Component | Lines of C (est.) |
|-----------|--------------------|
| tml.h | 100 |
| heap.c | 80 |
| symbol.c | 50 |
| env.c | 60 |
| read.c | 150 |
| print.c | 80 |
| eval.c | 200 |
| builtin.c | 150 |
| gc.c | 100 |
| platform_host.c | 30 |
| platform_cor24.c | 40 |
| main.c | 60 |
| **Total** | **~1100** |

Plus ~100 lines of Lisp for stdlib.

---

## Current Status (as of v0.1.0)

### Implemented

| Feature | Details |
|---------|---------|
| **Core evaluator** | 8 special forms: quote, quasiquote, if, define, set!, lambda, defmacro, begin |
| **41 C primitives** | Arithmetic, list ops, strings, I/O, GC, metaprogramming |
| **5 compiled preludes** | minimal (6 defs), standard (~50), full (~90), scheme (~55), bare (0) |
| **Tail-call optimization** | if, begin, closure application |
| **Conservative GC** | Stack scanning, iterative cdr marking |
| **Quasiquote** | `` ` ``, `,`, `,@` with qq_expand |
| **Strings** | Immutable, 119-char literals, 6 primitives |
| **Lazy sequences** | lazy-cons, lazy-map, lazy-filter, iterate, lazy-range |
| **Threading macros** | `->`, `->>` (two-arg) |
| **Anaphoric macros** | aif, awhen, aand |
| **Variadic functions** | `(lambda args ...)`, `(lambda (a . rest) ...)` |
| **set!** | Mutable assignment in local and global env |
| **Metaprogramming** | eval, macroexpand-1, macroexpand, gensym, symbol↔string |
| **Comments** | `;` line, `#_` datum, `(comment ...)` form |
| **Multi-line REPL** | Paren-depth tracking across newlines |
| **Error handling** | Type checks, div-by-zero, car/cdr safety; PANIC diagnostics |
| **Reader extensions** | `#t`/`#f`, `#x` hex, `#_` datum comment |
| **19 demos** | Language features, bottles variants, Scheme, fixed-point |
| **CLI wrapper** | `bin/tml24c` with --help, --version |

### Prelude tiers

| Prelude | Defs | Build | Run |
|---------|------|-------|-----|
| minimal | 6 | `just build-minimal` | `just run-minimal` |
| standard | ~50 | `just build-standard` | `just run` |
| full | ~90 | `just build-full` | `just run-full` |
| scheme | ~55 | `just build-scheme` | `just run-scheme` |
| bare | 0 | `just build-bare` | `just run-custom <prelude.l24>` |

---

## Next Steps

### Priority 1: Quick wins (no external dependencies)

| Feature | Effort | Description |
|---------|--------|-------------|
| `letrec` | Small | Self-referencing local bindings. Expand to `let` + `set!`: `(let ((f nil)) (set! f (lambda ...)) body)` |
| `(define (f x) body)` shorthand | Small | Detect `(define (name . params) body)` in eval, expand to `(define name (lambda params body))` |
| `try`/`guard` error recovery | Small | Global error flag + macro. `(try expr fallback)` checks flag after eval |
| Multi-methods | Small | Pure Lisp with alist dispatch. `set!` makes defmethod cleaner |
| `group-by` / `frequencies` | Small | Pure Lisp with reduce + alist |

### Priority 2: Medium features (tml24c C changes)

| Feature | Effort | Description |
|---------|--------|-------------|
| Flat vectors | Medium (~80 lines) | New ETYPE_VECTOR, contiguous cells, `vector-ref`/`vector-set!`/`vector-length` |
| Debug/trace primitives | Medium | Instruction counter, env dump, GC stats, trace on/off |
| String pool GC | Medium | Compact or relocate dead strings during collection |
| `read` primitive | Small | Expose `read_str` as a Lisp primitive for runtime parsing |

### Priority 3: cor24-rs changes (feature requests written)

| Feature | Request | Description |
|---------|---------|-------------|
| `--stack-kilobytes` | `docs/required-stack-changes.md` | 3/8/custom KB stack for larger preludes |
| Timer I/O register | `cor24-rs/docs/feature-timer-register.md` | Millisecond counter at 0xFF0200 for speed-independent delay |
| Watchdog / exit codes | `cor24-rs/docs/feature-watchdog.md` | Distinguish clean halt vs PANIC vs timeout; detect stuck programs |
| `--echo` improvements | `cor24-rs/docs/feature-terminal-echo.md` | Backspace handling, control char suppression |

### Priority 4: Larger efforts

| Feature | Effort | Description |
|---------|--------|-------------|
| Persistent vectors | Large (~500 lines) | Clojure-style HAMT, O(log32 n) updates with structural sharing |
| Hash maps | Large (~600 lines) | Persistent hash maps via bitmap-indexed tries |
| Software floats | Medium | Requires tc24r + emulator support first |
| Scheme `call/cc` | Very large | Stack reification — likely not feasible on COR24 |
| Hygienic macros | Very large | syntax-rules/syntax-case (~200+ lines) |

### Interactive terminal improvements

| Feature | Effort | Description |
|---------|--------|-------------|
| Line editing (backspace) | Small | Handle BS/DEL in read_line |
| Cursor movement (arrows) | Medium | Terminal escape sequences |
| History (up/down) | Large | Ring buffer of previous lines |

See also: `docs/futures.md` (numeric types, data structures), `docs/scheme-todo-items.md` (R7RS compatibility)
