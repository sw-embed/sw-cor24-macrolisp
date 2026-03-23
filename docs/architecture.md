# Architecture: Tiny Macro Lisp (tml24c)

## System Overview

```
                    ┌──────────────────────────────────────────────┐
                    │              tml24c Runtime                   │
                    │                                              │
  UART input ──────►  Reader ──► S-expression (Value tree)        │
                    │                    │                          │
                    │                    ▼                          │
                    │              Macro Expand                     │
                    │                    │                          │
                    │                    ▼                          │
                    │               Evaluator ◄──── Environment    │
                    │                    │            (lexical)     │
                    │                    ▼                          │
                    │               Printer ────────► UART output  │
                    │                                              │
                    │          ┌─── Heap ───┐                      │
                    │          │ cons pairs  │                      │
                    │          │ symbols     │◄──── GC (mark-sweep) │
                    │          │ strings     │                      │
                    │          │ closures    │                      │
                    │          │ macros      │                      │
                    │          └─────────────┘                      │
                    │                                              │
                    │         Platform Abstraction Layer            │
                    └──────────┬───────────────────────┬───────────┘
                               │                       │
                    ┌──────────▼──────┐     ┌─────────▼──────────┐
                    │  Host (gcc)     │     │  COR24 (tc24r)     │
                    │  stdio putchar  │     │  UART MMIO         │
                    │  malloc arena   │     │  SRAM heap         │
                    └─────────────────┘     └────────────────────┘
```

## Components

### Reader (`read.c`)
Converts text input into Value trees (S-expressions).

**Tokens:** `(`, `)`, `'`, `` ` ``, `,`, `,@`, `"string"`, integers, symbols.

**Produces:** Tagged Value objects — fixnums (immediate), symbols and lists (heap-allocated cons pairs).

### Evaluator (`eval.c`)
Tree-walking interpreter. Dispatches on value type and special form tag.

**Special forms:**
- `quote` — return unevaluated
- `if` — conditional
- `define` — bind in current environment
- `set!` — mutate existing binding
- `lambda` — create closure (captures environment)
- `defmacro` — create macro object
- `do` / `progn` — sequential evaluation
- `let` / `let*` — local bindings (may be macros over lambda)
- `quasiquote` — template with unquote/splice

**Function application:** Evaluate operator, evaluate arguments, extend closure's captured environment with bindings, evaluate body.

### Macro Expander (in `eval.c`)
Before normal evaluation, check if the operator is a macro. If so:
1. Apply macro function to **unevaluated** arguments
2. Get expanded form
3. Evaluate the expansion

Single-pass expand-then-eval (no separate expansion phase initially).

### Environment (`env.c`)
Lexical scope via chained environment frames.

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│ frame 3     │────►│ frame 2     │────►│ global      │
│ x = 10     │     │ y = 20      │     │ + = <prim>  │
│ z = 30     │     │             │     │ cons = <p>  │
└─────────────┘     └─────────────┘     └─────────────┘
```

Each frame is a list of (symbol . value) pairs linked to a parent frame. Lookup walks the chain outward.

### Garbage Collector (`gc.c`)
Mark-sweep over a single contiguous heap.

**Roots:**
- Global environment
- Current evaluation stack (temp roots)
- Symbol table
- Reader/printer temporaries

**Mark phase:** Recursive traversal from roots, setting mark bit on each reachable object.

**Sweep phase:** Linear scan of heap, free unmarked objects to free list.

**Trigger:** When allocation fails (heap exhausted), run GC. If still insufficient, report out-of-memory.

### Printer (`print.c`)
Converts Value trees back to text. Handles: fixnums, symbols, lists (proper and dotted), strings, `#<closure>`, `#<primitive>`, `#<macro>`.

### Symbol Table (`symbol.c`)
Interned symbols — each unique name has exactly one symbol object. Comparison is pointer equality.

### Heap (`heap.c`)
Fixed-size arena of cells. Initially bump allocation (no GC). Later: free-list with mark-sweep.

### Platform Abstraction (`platform.h`)
```c
void platform_putchar(int ch);
int  platform_getchar(void);
void platform_halt(int code);
```

**Host:** Maps to `putchar`/`getchar`/`exit`.
**COR24:** Maps to UART MMIO at 0xFF0100 with TX-busy polling.

## Memory Model

### Host Build
Standard C memory. Heap is a static array (`Value heap[HEAP_SIZE]`). Stack is C call stack.

### COR24 Build

```
0x000000 ┌─────────────────┐
         │ Code (.text)     │  Program + string literals
         ├─────────────────┤
         │ Data (.data)     │  Initialized globals
         ├─────────────────┤
         │ BSS (.comm)      │  Symbol table, heap array
         │                  │
         │ ▼ Heap grows up  │
         │                  │
         │                  │
0x0FFFFF └─────────────────┘  End of SRAM (1 MB)

0xFEE000 ┌─────────────────┐
         │                  │
         │ ▲ Stack grows dn │
         │                  │
0xFEEC00 └─────────────────┘  Initial SP (top of EBR)

0xFF0100   UART_DATA (TX/RX)
0xFF0101   UART_STAT
```

**Heap sizing:** With 1 MB SRAM, after code (~10–20 KB estimated), the heap can be 200–900 KB. Each cons cell = 6 bytes (two 24-bit tagged values). A 64K-cell heap = 384 KB.

### Tagged Value Representation

Every Lisp value is a single 24-bit word with tag bits in the low 2 bits:

| Tag (bits 1:0) | Type | Payload (bits 23:2) |
|-----------------|------|---------------------|
| 00 | Fixnum | 22-bit signed integer |
| 01 | Cons pointer | Heap address >> 1 (word-aligned) |
| 10 | Symbol pointer | Heap address >> 1 |
| 11 | Other pointer | Closure, string, primitive, macro |

**Fixnum range:** -2,097,152 to 2,097,151 (22-bit signed).

**Special values:**
- `NIL` = symbol pointer to the interned `nil` symbol (or a dedicated sentinel)
- `T` = symbol pointer to the interned `t` symbol

### Heap Object Layout

**Cons cell (6 bytes):**
```
┌──────────┬──────────┐
│ car (3B) │ cdr (3B) │  Both are tagged Values
└──────────┴──────────┘
```

**Symbol (6+ bytes):**
```
┌──────────┬──────────┐
│ name ptr │ next sym │  name = pointer to string, next = symbol chain
└──────────┴──────────┘
```

**Closure (9 bytes):**
```
┌──────────┬──────────┬──────────┐
│ params   │ body     │ env      │  All tagged Values (lists/pointers)
└──────────┴──────────┴──────────┘
```

**String (3 + n bytes):**
```
┌──────────┬──────────────────┐
│ length   │ chars[0..n-1]    │  length = fixnum, chars = byte array
└──────────┴──────────────────┘
```

**GC header:** Each heap object needs a mark bit + type tag for the collector. Options:
- Steal 1 bit from the object header
- Use a separate bitmap (1 bit per cell)
- Use a type byte prefix before each object

Design decision deferred to implementation — the separate bitmap approach avoids disturbing object layout.

## Data Flow

### REPL Loop
```
loop:
  print("> ")
  expr = read(stdin)
  if expr == EOF: halt
  result = eval(expr, global_env)
  print(result)
  println()
  goto loop
```

### Macro Expansion (in eval)
```
eval(expr, env):
  if expr is symbol: lookup(expr, env)
  if expr is atom: return expr
  // expr is a list: (op args...)
  op_val = eval(car(expr), env)
  if op_val is macro:
    expanded = apply(op_val, cdr(expr))  // unevaluated args
    return eval(expanded, env)           // eval the expansion
  else:
    args = eval_list(cdr(expr), env)
    return apply(op_val, args)
```

## Build System

### Host Build
```makefile
CC = cc
CFLAGS = -Wall -Wextra -Werror -std=c11
SRCS = main.c eval.c read.c print.c heap.c symbol.c env.c builtin.c gc.c platform_host.c
```

### COR24 Build (future)
```makefile
CC = tc24r
# Single-file compilation: concatenate or #include all sources
# Output: tml24c.s → as24 → tml24c.lgo
```

tc24r requires single-file input, so the COR24 build will either:
1. Use `#include` to pull all `.c` files into one translation unit, or
2. Concatenate sources with a build script

## Dependencies

- **cor24-rs** (assembler + emulator) — for COR24 target builds and testing
- **tc24r** (C compiler) — for COR24 target compilation
- **gcc/clang** — for host development and testing
- **make** — build system
