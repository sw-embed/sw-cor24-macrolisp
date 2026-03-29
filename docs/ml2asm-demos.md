# Inline Assembly Demos

These demos use the `(asm ...)` special form in the Macro Lisp cross-compiler
to produce standalone COR24 programs mixing Lisp and assembly.

## Prerequisites

- `tc24r` (Tiny COR24 compiler) on PATH
- `cor24-run` (COR24 assembler/emulator) on PATH
- Working directory: `tml24c/`

## Demo: ISR Echo (demos/isr-echo.l24)

Interrupt-driven UART echo. An ISR written in inline asm reads each
UART byte and calls a compiled Lisp handler, which calls an asm
`uart-putc` helper to echo it back.

Flow: UART RX interrupt -> ISR (asm) -> handle-char (Lisp) -> uart-putc (asm) -> UART TX

### Compile to .s

```bash
just compile demos/isr-echo.l24
```

Output is the generated COR24 assembly. Also saved to `build/compiled.s`.

### Run with UART input

```bash
just run-compiled-uart demos/isr-echo.l24 "Hello"
```

Or use the named demo target:

```bash
just demo-isr-echo
```

### Run with dump (debug)

After compiling, run the .s directly with cor24-run flags:

```bash
just compile demos/isr-echo.l24 > /dev/null
cor24-run --run build/compiled.s --speed 0 -n 10000000 -u "AB" --dump
```

Add `--trace 50` to see the last 50 instructions before halt:

```bash
cor24-run --run build/compiled.s --speed 0 -n 10000000 -u "AB" --dump --trace 50
```

Add `--step` to trace every instruction (very verbose):

```bash
cor24-run --run build/compiled.s --speed 0 -n 1000 -u "A" --step
```

### Run the regression test

```bash
just test-asm
```

Compiles isr-echo.l24, runs it with input "AB", verifies "AB" is echoed back.

## Compile Pipeline

The compile pipeline has two stages:

1. **Lisp to .s**: The compiler driver (`build/compiler.s`) runs on
   cor24-run, reads .l24 from UART, calls `compile_program`, emits
   COR24 assembly to UART.

2. **Assemble and run**: cor24-run assembles the .s and runs it.

```
.l24 source
  |  grep -v '^;;' (strip comment-only lines)
  |  printf '\004' (append Ctrl-D for EOF)
  v
cor24-run --run build/compiler.s --terminal
  |  (compiler reads Lisp, emits .s)
  v
build/compiled.s
  |
  v
cor24-run --run build/compiled.s [-u "input"] [--dump] [--trace N]
```

### Justfile targets

| Command | Description |
|---------|-------------|
| `just build-compiler` | Build the compiler driver |
| `just compile <file>` | Compile .l24 to .s (stdout + build/compiled.s) |
| `just run-compiled <file>` | Compile and run |
| `just run-compiled-uart <file> <input>` | Compile and run with UART input |
| `just test-asm` | Regression test (compile + run + verify output) |
| `just demo-isr-echo` | ISR echo demo with "Hello, COR24!" |

## The (asm ...) Form

Strings are emitted verbatim (no auto-indentation). Symbols emit their
compiler-generated global label (`_G<n>`), enabling asm to reference
compiled Lisp functions.

```scheme
;; Strings: emitted as-is, one line each
(asm "        nop"
     "        nop")

;; Labels: no indentation needed
(asm "_my_label:"
     "        bra _my_label")

;; Symbol reference: emits _G<n> for the Lisp global
(asm "        la   r0," my-function
     "        lw   r0,0(r0)")

;; Define an asm routine callable from Lisp (standard ABI)
(asm "_my_helper:"
     "        push fp"
     "        push r2"
     "        push r1"
     "        mov  fp,sp"
     "        lw   r0,9(fp)"
     "        ; ... do work, result in r0 ..."
     "        mov  sp,fp"
     "        pop  r1"
     "        pop  r2"
     "        pop  fp"
     "        jmp  (r1)")

;; Wire it as a Lisp-callable function
(define my-helper (asm "        la   r0,_my_helper"))
```
