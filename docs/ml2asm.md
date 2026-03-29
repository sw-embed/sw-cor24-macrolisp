# Macro Lisp to Assembly Integration

Three approaches for calling COR24 assembly from Macro Lisp, ordered by
implementation timeline. Each builds on the previous.

## Background

Macro Lisp (tml24c) runs on COR24, a 24-bit RISC processor. The C and
Pascal compilers are hosted on Mac/Linux (compiled Rust) and cannot run
on-target. For sw-os development, we need Lisp programs to access
hardware registers, call native routines, and eventually be fully
self-hosted without a cross-compiler in the loop.

Existing hardware access: `peek` (byte read) and `poke` (byte write)
provide raw memory-mapped I/O, but they operate one byte at a time and
cannot access registers or execute arbitrary instructions.

### COR24 Calling Convention

| Register | Role              | Saved by |
|----------|-------------------|----------|
| r0       | Return value      | Caller   |
| r1       | Return address    | Callee   |
| r2       | Register variable | Callee   |
| fp (r3)  | Frame pointer     | Callee   |
| sp (r4)  | Stack pointer     | —        |
| z (r5)   | Zero constant     | —        |

Arguments are passed on the stack, pushed right-to-left. Return value
in r0. Standard prologue saves fp, r2, r1 then sets fp = sp.

### COR24 Instruction Encoding

Instructions are variable-length: 1 byte (register ops), 2 bytes
(register + 8-bit immediate), or 4 bytes (register + 24-bit address).
Encoded via ROM lookup from a single opcode byte.

---

## Approach 1: Cross-Compiled Inline Assembly

**Status:** Not yet implemented. First priority.

**Use case:** Developing on the host (Mac/Linux), deploying compiled `.s`
files to COR24. The Lisp compiler (`compile.h`) emits COR24 assembly;
inline `asm` lets Lisp source embed raw instructions.

### Design

Add `asm` as a special form recognized by the compiler:

```scheme
;; Single instruction
(asm "la r0,#xFF0000")

;; Multiple instructions
(asm "la r0,#xFF0000"
     "lw r1,0(r0)"
     "add r1,r1"
     "sw r1,0(r0)")

;; In context: toggle LEDs
(define (toggle-leds)
  (asm "la r0,#xFF0000"    ; IO-LED address
       "lw r1,0(r0)"       ; read current
       "not r1"             ; flip bits
       "sw r1,0(r0)"))     ; write back
```

The compiler emits each string as a literal line in the assembly output.
No parsing, no validation — the assembler (as24) handles that.

### Implementation

In `compile.h`, add a check in `cexpr()`:

```c
if (head == sym_asm) {
    /* Emit each string argument as a raw assembly line */
    int strs = args;
    while (!IS_NIL(strs)) {
        char *s = string_data(car(strs));
        /* emit with standard indentation */
        puts_str("        ");
        puts_str(s);
        putc_uart('\n');
        strs = cdr(strs);
    }
    return;
}
```

Register `sym_asm = intern("asm")` in `eval_init()`.

### Constraints

- Only available when cross-compiling via tc24r on the host.
- No register allocation awareness — the programmer must respect the
  calling convention (r0 is return value, r1/r2/fp are callee-saved).
- The compiler does not track what `asm` does to registers or the stack.
  Corrupting fp or sp will crash.
- Result of an `(asm ...)` expression is whatever is in r0 when it ends.

### Register access from inline asm

To pass Lisp values into assembly and get results back:

```scheme
;; Read a 24-bit word from an I/O address
(define (peek-word addr)
  ;; addr is evaluated, result in r0
  ;; asm takes over from there
  (asm "lw r0,0(r0)"))   ; r0 = *(int*)r0

;; Write a 24-bit word
(define (poke-word addr val)
  ;; After standard prologue, addr is at fp+9, val at fp+12
  (asm "lw r0,9(fp)"      ; r0 = addr
       "lw r1,12(fp)"     ; r1 = val
       "sw r1,0(r0)"))    ; *addr = val
```

---

## Approach 2: Runtime FFI (`call-native`)

**Status:** Not yet implemented. Second priority.

**Use case:** The Lisp interpreter is running on COR24 (in sw-os or via
the REPL). Assembly routines have been assembled separately and loaded
into memory. Lisp code calls them by address.

### Design

A new primitive `call-native` invokes machine code at a given address:

```scheme
;; Call a routine at address with arguments
(call-native #x1000)           ; no args, return r0
(call-native #x1000 42)        ; one arg
(call-native #x1000 42 99)     ; two args
```

The primitive:
1. Pushes Lisp fixnum args onto the COR24 stack (right-to-left per ABI)
2. Calls `jsr` to the target address
3. Cleans up the stack
4. Returns r0 as a Lisp fixnum

### Implementation sketch

Add `PRIM_CALL_NATIVE` to eval.h:

```c
if (id == PRIM_CALL_NATIVE) {
    int addr = FIXNUM_VAL(a);
    int native_args = cdr(args);  /* remaining args after address */
    /* Push args, call addr, clean up, return r0 */
    /* This requires a small asm trampoline since we need to
       switch from the C stack convention to a raw call */
    ...
}
```

The trampoline is the tricky part — it needs to bridge between the
interpreter's C calling convention and a raw COR24 function call. One
approach: a fixed trampoline routine compiled into the interpreter that
reads the address and args from known locations.

### Workflow

1. Write a `.s` file on-target (using the editor)
2. Assemble it with the on-target assembler (as24 or equivalent)
3. Load the binary at a known address (e.g., `#x2000`)
4. From Lisp: `(call-native #x2000 arg1 arg2)`

### Assembly routine requirements

Routines called via `call-native` must:
- Follow the standard COR24 calling convention (args on stack, return in r0)
- Save and restore r1, r2, fp if used
- Return via `jmp (r1)`

Example `.s` routine (add two numbers using registers):

```asm
; add_fast.s — add two 24-bit values
; Args: a at sp+3, b at sp+6 (after jsr pushes return addr)
_add_fast:
    push    fp
    push    r2
    push    r1
    mov     fp,sp
    lw      r0,9(fp)      ; a
    lw      r1,12(fp)     ; b
    add     r0,r1         ; r0 = a + b
    mov     sp,fp
    pop     r1
    pop     r2
    pop     fp
    jmp     (r1)
```

---

## Approach 3: On-Target Assembler in Lisp

**Status:** Future work. Third priority.

**Use case:** Full self-hosting. Write, assemble, and call native
routines entirely from the Lisp REPL on COR24. No host compiler needed.

### Design

A Lisp function `assemble` takes a list of symbolic instructions,
encodes them into a memory buffer, and returns the buffer address:

```scheme
(define my-add
  (assemble '((push fp)
              (push r2)
              (push r1)
              (mov fp sp)
              (lw r0 9 fp)
              (lw r1 12 fp)
              (add r0 r1)
              (mov sp fp)
              (pop r1)
              (pop r2)
              (pop fp)
              (jmp r1))))

(call-native my-add 21 21)  ;; => 42
```

### Why this is feasible on COR24

- Fixed instruction set with ~211 valid encodings
- Instructions are 1, 2, or 4 bytes — simple to emit
- Flat memory model — write bytes anywhere and execute them
- The encoding table (opcode byte to register/operation mapping) can be
  stored as a Lisp association list

### Implementation path

1. **Encoding table** — A Lisp alist mapping `(opcode ra rb)` triples to
   the 1-byte instruction encoding. Generated from the COR24 ISA spec.

2. **`assemble` function** — Written in Lisp. Walks the instruction list,
   looks up encodings, writes bytes to a buffer via `poke`, handles
   immediates and label resolution.

3. **Memory allocation** — A simple bump allocator for code buffers.
   Could be as simple as a global `*code-next*` pointer that advances.

4. **Label support** — Two-pass assembly: first pass collects label
   offsets, second pass emits with resolved addresses.

### Approximate size

The assembler would be roughly 50-100 lines of Lisp:
- ~20 lines for the encoding table lookup
- ~30 lines for instruction emission (1/2/4 byte variants)
- ~20 lines for the two-pass label resolution
- ~10 lines for the memory buffer management

### Combined with `call-native`

Once both Approach 2 and 3 are implemented, inline assembly in the
interpreter becomes sugar:

```scheme
(defmacro asm instructions
  `(call-native (assemble ',instructions)))
```

This gives the same `(asm ...)` syntax as the cross-compiled version
but runs entirely on-target.

---

## Summary

| Approach | Runs on | Requires | Priority |
|----------|---------|----------|----------|
| 1. Cross-compiled `(asm ...)` | Host | tc24r + as24 | Now |
| 2. `call-native` FFI | COR24 | On-target assembler | Next |
| 3. Lisp assembler + `(asm ...)` | COR24 | Approach 2 | Future |

The progression: cross-compile first to build and test sw-os
primitives, then add runtime FFI for on-target development, then a
self-hosted assembler for full independence from the host toolchain.
