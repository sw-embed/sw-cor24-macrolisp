# Product Requirements: Tiny Macro Lisp (tml24c)

## Purpose

A minimal Lisp-1 with lexical scope, unhygienic `defmacro`, closures, and mark-sweep GC. Written in C, targeting COR24 24-bit RISC architecture. Serves three goals:

1. **Learn** — First garbage-collected interpreter implementation
2. **Validate** — Stress-test tc24r C compiler, cor24-rs assembler/emulator on non-trivial software
3. **Use** — Provide a programmable language for COR24 embedded apps

## Target Demos

### Must demonstrate
- **Anaphoric macros:** `aif`, `awhen` — the flagship macro system demo
- **List processing:** `map`, `filter`, `foldl` over cons lists
- **Recursive functions:** Fibonacci, factorial, list-length, append
- **Closures:** `make-adder`, counter with captured state
- **Arithmetic:** Integer math on 22-bit fixnums (tagged 24-bit words)
- **REPL:** Interactive read-eval-print loop over UART
- **File/string eval:** Load and evaluate Lisp source from embedded strings

### Should demonstrate
- **String operations:** Literals, concatenation, comparison, print
- **Bootstrap library:** `when`, `unless`, `cond`, `and`, `or` defined in Lisp
- **Garbage collection:** Programs that allocate heavily and survive GC cycles
- **Quasiquote:** Backquote/unquote/unquote-splicing for macro definitions

### Nice to have
- **Board demo:** LED control or UART echo app written in Lisp
- **Threading macros:** `->`, `->>` (Clojure-inspired)
- **`macroexpand-1`:** Inspect macro expansion interactively

## Constraints

### Hardware
- **ISA:** COR24 — 24-bit RISC, 8 registers (3 GP), variable-length instructions
- **Memory:** 1 MB SRAM + 3 KB EBR; heap must fit in available SRAM after code
- **I/O:** UART only (0xFF0100), no filesystem
- **No hardware divide** — division via runtime helpers (repeated subtraction)
- **No FPU** — integer-only arithmetic

### Toolchain
- **C compiler:** tc24r — C99 subset, single-file, no float, no varargs, no bitfields
- **Assembler:** cor24-rs `as24` — two-pass, LGO output
- **Emulator:** cor24-rs — step/run/breakpoint, UART capture
- **Host testing:** Must also compile with gcc/clang on host for development

### Language constraints (from research)
- No hygienic macros, no package system, no CLOS, no continuations
- No bignums, ratios, or complex numeric tower
- No bytecode VM initially — tree-walking interpreter
- No tail-call optimization initially (add if time permits)

## Success Criteria

### Phase 1: Host Bootstrap (MVP)
- [ ] Evaluator handles: quote, if, define, lambda, function application
- [ ] Reader parses: integers, symbols, lists, quote shorthand
- [ ] Printer outputs: integers, symbols, lists
- [ ] Builtins: cons, car, cdr, eq?, atom?, +, -, *, <, print
- [ ] Lexical closures work (make-adder test passes)
- [ ] Compiles and runs on host (gcc/clang)

### Phase 2: Full Language
- [ ] Mark-sweep GC handles allocation pressure
- [ ] `defmacro` + quasiquote + gensym work
- [ ] `when`, `unless`, `cond`, `and`, `or` defined in Lisp
- [ ] `aif`, `awhen` anaphoric macros work
- [ ] `map`, `filter`, `foldl` work
- [ ] String type with basic operations

### Phase 3: COR24 Target
- [ ] Compiles with tc24r (single-file build)
- [ ] Runs on cor24-rs emulator
- [ ] UART REPL works
- [ ] GC stress test passes on emulator
- [ ] Macro demo runs end-to-end

### Phase 4: Integration & Polish
- [ ] Board demo (FPGA or emulator)
- [ ] Bootstrap library loaded from embedded strings
- [ ] Documentation complete
