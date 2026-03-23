# Detailed Design: Tiny Macro Lisp (tml24c)

## Data Representation

### Tagged Values

All Lisp values are represented as a single 24-bit word (`typedef int Value;` on COR24, `typedef int32_t Value;` on host).

```
Bit layout (24 bits):
[23 ..................... 2][1][0]
         payload            tag
```

| Tag | Type | Payload |
|-----|------|---------|
| 00 | Fixnum | 22-bit signed integer (arithmetic shift right 2 to decode) |
| 01 | Cons | Heap index (cell number, not byte address) |
| 10 | Symbol | Heap index |
| 11 | Extended | Heap index → object with runtime type header |

**Fixnum encoding:**
```c
#define MAKE_FIXNUM(n)   (((n) << 2) | TAG_FIXNUM)
#define FIXNUM_VAL(v)    ((v) >> 2)           // arithmetic shift preserves sign
#define IS_FIXNUM(v)     (((v) & 3) == TAG_FIXNUM)
```

**Pointer encoding:**
```c
#define MAKE_CONS(idx)   (((idx) << 2) | TAG_CONS)
#define CONS_IDX(v)      ((v) >> 2)
#define IS_CONS(v)       (((v) & 3) == TAG_CONS)
```

### Special Values

```c
#define NIL_VAL          MAKE_SYMBOL(0)   // symbol index 0 = nil
#define T_VAL            MAKE_SYMBOL(1)   // symbol index 1 = t
#define IS_NIL(v)        ((v) == NIL_VAL)
#define IS_TRUE(v)       (!IS_NIL(v))     // everything non-nil is true
```

### Fixnum Range

22-bit signed: -2,097,152 to 2,097,151. Sufficient for demos, loop counters, list indices. Overflow wraps silently (no bignum fallback).

## Heap Layout

### Cell Array

The heap is a fixed array of cells. Each cell is 6 bytes (two 24-bit words).

```c
typedef struct {
    Value car;
    Value cdr;
} Cell;

#define HEAP_SIZE 8192   // tunable; 8K cells = 48 KB
Cell heap[HEAP_SIZE];
```

All heap-allocated objects occupy one or more cells:

| Object | Cells | Layout |
|--------|-------|--------|
| Cons pair | 1 | car=first, cdr=rest |
| Symbol | 1 | car=name_string, cdr=next_symbol |
| Closure | 2 | cell0: car=params, cdr=body; cell1: car=env, cdr=type_tag |
| Macro | 2 | Same as closure, different type_tag |
| Primitive | 1 | car=function_index, cdr=type_tag |
| String | 1+ | car=length, cdr=pointer to char storage |

### Free List

After GC initialization, all unused cells form a singly-linked free list via the `cdr` field:

```
free_list → cell[N] → cell[M] → cell[K] → NIL
```

Allocation pops from the free list. Sweep rebuilds it.

### Extended Object Type Tags

Objects tagged `11` (extended) carry a type discriminator in their second cell's `cdr`:

```c
#define ETYPE_CLOSURE    MAKE_FIXNUM(1)
#define ETYPE_MACRO      MAKE_FIXNUM(2)
#define ETYPE_PRIMITIVE  MAKE_FIXNUM(3)
#define ETYPE_STRING     MAKE_FIXNUM(4)
```

## Environment Structure

### Frame Representation

An environment is a tagged cons pointer to a list of frames. Each frame is a cons pair:
- `car` = association list of (symbol . value) pairs
- `cdr` = parent environment (or NIL for global)

```
env → (frame . parent_env)
       │
       ▼
      ((x . 10) (y . 20) . nil)
```

### Operations

```c
Value env_lookup(Value sym, Value env);     // walk chain, error if unbound
Value env_define(Value sym, Value val, Value env);  // add to current frame
Value env_set(Value sym, Value val, Value env);     // update existing binding
Value env_extend(Value params, Value args, Value env); // new frame with bindings
```

### Global Environment

The global environment is a single frame containing all built-in primitives and user `define`d values. Initialized at startup.

## Closure Representation

A closure captures: parameter list, body, and the environment at definition time.

```
Extended object (2 cells):
  Cell 0: car = param_list,  cdr = body
  Cell 1: car = captured_env, cdr = ETYPE_CLOSURE
```

**Application:**
```
apply(closure, args):
  new_env = env_extend(closure.params, args, closure.env)
  return eval_body(closure.body, new_env)
```

## Macro Representation

Identical to closure but tagged `ETYPE_MACRO`. During eval, if the operator resolves to a macro:

1. **Do not** evaluate the arguments
2. Apply the macro function to the raw argument forms
3. Evaluate the resulting expansion in the original environment

```
eval((op . args), env):
  op_val = eval(op, env)
  if is_macro(op_val):
    expanded = apply_macro(op_val, args)  // args NOT evaluated
    return eval(expanded, env)
```

## Macro Expansion Strategy

### defmacro

```lisp
(defmacro when (test . body)
  `(if ,test (do ,@body) nil))
```

Binds a macro object in the global environment. The macro's body is a closure that receives unevaluated argument forms and returns a new form.

### Quasiquote

Implemented in the reader as syntactic sugar:

| Syntax | Reads as |
|--------|----------|
| `` `x `` | `(quasiquote x)` |
| `,x` | `(unquote x)` |
| `,@x` | `(unquote-splicing x)` |

The evaluator handles `quasiquote` as a special form that recursively processes the template:
- Literal forms → quoted
- `(unquote x)` → evaluate x
- `(unquote-splicing x)` → evaluate x, splice into list

### gensym

```c
static int gensym_counter = 0;
Value lisp_gensym(void) {
    // Create uninterned symbol: G__0, G__1, etc.
}
```

Generates unique symbols to avoid variable capture in macros.

## Garbage Collector

### Algorithm: Mark-Sweep

**When triggered:** Allocation fails (free list empty).

**Mark phase:**
```
mark(val):
  if val is fixnum: return        // immediate, not on heap
  idx = heap_index(val)
  if mark_bit[idx]: return        // already marked
  mark_bit[idx] = 1
  if is_cons(val) or is_symbol(val):
    mark(heap[idx].car)
    mark(heap[idx].cdr)
  if is_extended(val):
    mark(heap[idx].car)
    mark(heap[idx].cdr)
    mark(heap[idx+1].car)         // second cell
    mark(heap[idx+1].cdr)
```

**Sweep phase:**
```
sweep():
  free_list = NIL
  for i = HEAP_SIZE-1 downto 0:
    if mark_bit[i]:
      mark_bit[i] = 0            // clear for next cycle
    else:
      heap[i].cdr = free_list    // add to free list
      free_list = i
```

### Mark Bit Storage

Separate bitmap array: `char mark_bits[(HEAP_SIZE + 7) / 8]`

One bit per cell. Avoids disturbing object layout. Costs HEAP_SIZE/8 bytes (1 KB for 8K cells).

### Root Enumeration

Roots that must be scanned:
1. `global_env` — the global environment
2. `temp_roots[]` — a small stack of values being constructed (reader, eval temps)
3. `symbol_table` — the interned symbol list
4. Evaluation stack — values on the C call stack during recursive eval

**C stack scanning strategy:** Use a "temp root" stack. Before any allocation that might trigger GC, push live values onto `temp_roots`. Pop after use.

```c
#define PUSH_ROOT(v)  (temp_roots[temp_root_count++] = (v))
#define POP_ROOT()    (temp_root_count--)
```

This avoids conservative stack scanning.

## Special Forms

| Form | Syntax | Semantics |
|------|--------|-----------|
| `quote` | `(quote x)` or `'x` | Return x unevaluated |
| `if` | `(if test then else)` | Conditional; else optional (defaults to nil) |
| `define` | `(define sym expr)` | Bind in current env |
| `set!` | `(set! sym expr)` | Mutate existing binding |
| `lambda` | `(lambda (params) body...)` | Create closure |
| `defmacro` | `(defmacro name (params) body...)` | Create macro |
| `do` | `(do expr1 expr2 ...)` | Evaluate sequentially, return last |
| `let` | `(let ((x 1) (y 2)) body...)` | Local bindings (parallel) |
| `let*` | `(let* ((x 1) (y x)) body...)` | Local bindings (sequential) |
| `quasiquote` | `` `(a ,b ,@c) `` | Template with interpolation |

`let` and `let*` may be implemented as macros expanding to lambda applications, or as direct special forms. Direct special forms are simpler for the evaluator.

## Built-in Primitives

### List Operations
- `cons`, `car`, `cdr`
- `list` (variadic: `(list 1 2 3)`)

### Predicates
- `eq?` — pointer equality
- `null?` — is nil
- `atom?` — not a cons pair
- `symbol?`, `number?`, `string?`, `pair?`

### Arithmetic
- `+`, `-`, `*`, `/`, `mod`
- `<`, `<=`, `>`, `>=`, `=` (numeric equality)

### I/O
- `print` — print value without newline
- `println` — print value with newline
- `read` — read one S-expression from input
- `display` — print strings without quotes

### Meta
- `gensym` — generate unique symbol
- `macroexpand-1` — expand one macro step
- `eval` — evaluate a form
- `apply` — apply function to argument list

### String (Phase 2)
- `string-length`, `string-ref`, `string-append`
- `string=?`, `string<?`
- `number->string`, `symbol->string`

## Error Handling

Minimal error handling for bootstrap:
- Print error message to UART/stdout
- Halt (COR24) or longjmp to REPL top-level (host)

No condition/restart system. No stack traces initially.

```c
void lisp_error(const char *msg, Value val);
// Prints: "Error: <msg> <val>" then halts or returns to REPL
```

On host, use `setjmp`/`longjmp` for error recovery to REPL. On COR24, initially just halt; later add REPL recovery.

## Reader Design

### Tokenizer States
1. Skip whitespace and comments (`;` to end of line)
2. `(` → open paren
3. `)` → close paren
4. `'` → read next, wrap in `(quote ...)`
5. `` ` `` → read next, wrap in `(quasiquote ...)`
6. `,` → peek for `@`; wrap in `(unquote ...)` or `(unquote-splicing ...)`
7. `"` → read string literal with escape sequences
8. `-` or digit → read integer
9. Other → read symbol (alphanumeric + `!?*+-/<>=_`)

### List Reader
```
read_list():
  tok = peek()
  if tok == ')': consume, return NIL
  first = read_expr()
  rest = read_list()
  return cons(first, rest)
```

No dotted-pair syntax initially. Add later if needed.

## File Organization

```
tml24c/
├── CLAUDE.md
├── Makefile
├── docs/
│   ├── research.txt
│   ├── architecture.md
│   ├── prd.md
│   ├── design.md
│   ├── plan.md
│   └── stats.md
├── src/
│   ├── tml.h          -- Value type, tags, macros, all declarations
│   ├── heap.c         -- Cell array, allocation, free list
│   ├── symbol.c       -- Symbol interning
│   ├── env.c          -- Environment frames
│   ├── read.c         -- Reader/tokenizer
│   ├── print.c        -- Printer
│   ├── eval.c         -- Evaluator + macro expansion
│   ├── builtin.c      -- Built-in primitive functions
│   ├── gc.c           -- Mark-sweep garbage collector
│   ├── platform_host.c -- Host I/O (stdio)
│   ├── platform_cor24.c -- COR24 I/O (UART MMIO)
│   └── main.c         -- Init, REPL loop, test harness
├── lib/
│   └── stdlib.lisp    -- Bootstrap library (when, unless, aif, map, etc.)
└── tests/
    ├── test_eval.lisp -- Evaluator tests
    ├── test_gc.lisp   -- GC stress tests
    └── test_macro.lisp -- Macro tests
```
