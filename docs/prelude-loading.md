# Prelude Loading Architecture

## Current Approach

Each prelude variant (standard, full, scheme) is a C header file containing
`eval_str()` calls that read and evaluate Lisp definitions at startup:

```c
void load_prelude() {
    eval_str("(define (map f lst) ...)");
    eval_str("(define (filter p lst) ...)");
    // ... 60-90 definitions
}
```

Each `eval_str` call: sets `read_ptr` → reads one expression → evaluates it
(which may trigger macro expansion) → extends `global_env`.

**Cost:** ~6-7M COR24 instructions for the standard prelude (~70 definitions).
The bare REPL (no prelude) starts in ~100K instructions.

## Batch Loading: load_string()

`load_string(char *s)` reads and evaluates multiple expressions from a single
string, avoiding per-call C function overhead:

```c
void load_string(char *s) {
    read_ptr = s;
    while (*read_ptr) {
        skip_whitespace();
        if (*read_ptr == 0) return;
        int expr = read_expr();
        eval(expr, global_env);
    }
}
```

**Limitation:** tc24r (the COR24 C compiler) doesn't support C string literal
concatenation (`"abc" "def"`), so the prelude can't be written as a single
`load_string()` call in a header. `load_string` is available for REPL use
and for loading .l24 files programmatically.

## Future: Pre-assembled Binary Loading

The cor24-rs emulator has planned support for loading pre-assembled binaries
(bypassing the assembler step). This would enable:

1. **Assemble once, load many** — the prelude's assembly output would be
   cached as a binary blob, eliminating the ~331K-line assembly step.

2. **Heap snapshots** — capture the interpreter state after prelude loading
   and serialize the heap. Startup would be: load binary → restore heap →
   enter REPL. This would reduce prelude cost from ~7M instructions to ~0.

3. **Split preludes** — load a core prelude as a binary, then extend with
   user definitions via UART. Only the user code pays the read+eval cost.

## Prelude Variants

| Variant | Definitions | Use case |
|---------|-------------|----------|
| `prelude-minimal.h` | 6 | Comparison operators only |
| `prelude-standard.h` | ~70 | Full language: list ops, macros, error handling, parameters |
| `prelude-full.h` | ~110 | Standard + lazy sequences, combinators, advanced utilities |
| `prelude-scheme.h` | ~85 | R7RS naming: let*, cond/else, boolean?, equal? |

## Instruction Budgets

| Operation | Instructions | Notes |
|-----------|-------------|-------|
| Bare REPL startup | ~100K | heap/gc/symbol/eval init |
| Standard prelude | ~7M | 70 read+eval cycles |
| Full prelude | ~12M | 110 read+eval cycles |
| `just test` suite | ~50M | 5 test suites |
| `just eval` file | ~500M budget | prelude + file eval |
| `just demo-bottles` | ~500M budget | 99 bottles output |
