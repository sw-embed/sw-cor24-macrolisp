# Multi-Module Programs

How to compile and link multiple modules into one COR24 program.
Each module is assembled at a different base address and loaded
into memory by the emulator. The main program (compiled Lisp)
dispatches to them by fixed address.

## Concept

```
demos/multi/main.l24  (Lisp)  →  compile  →  build/main.s    →  runs at 0x0000
demos/multi/uart.s    (asm)   →  assemble →  build/uart.bin  →  loaded at 0x1000
demos/multi/spi.s     (asm)   →  assemble →  build/spi.bin   →  loaded at 0x2000
demos/multi/i2c.s     (asm)   →  assemble →  build/i2c.bin   →  loaded at 0x3000
demos/multi/gpio.s    (asm)   →  assemble →  build/gpio.bin  →  loaded at 0x4000
demos/multi/timer.s   (asm)   →  assemble →  build/timer.bin →  loaded at 0x5000
```

The emulator starts executing at 0x0000 (the compiled Lisp main).
Service modules don't run on their own — they sit in memory as
callable subroutines. The Lisp main calls them by address:

```scheme
(define uart-putc  (asm "        la   r0,#x1000"))
(define spi-xfer   (asm "        la   r0,#x2000"))
(define i2c-xfer   (asm "        la   r0,#x3000"))
(define gpio-op    (asm "        la   r0,#x4000"))
(define timer-op   (asm "        la   r0,#x5000"))

(spi-xfer)
(gpio-op)
```

## Running the Demo

### Quick run

```bash
just demo-multi
```

Output:
```
> SPI
> I2C
> GPIO
> TMR
```

Each module prints its name via uart-putc to prove the cross-module
call worked.

### Debug with dump and trace

After `just demo-multi` has built everything:

```bash
cor24-run --run build/main.s --load-binary build/uart.bin@0x1000 --load-binary build/spi.bin@0x2000 --load-binary build/i2c.bin@0x3000 --load-binary build/gpio.bin@0x4000 --load-binary build/timer.bin@0x5000 --speed 0 -n 10000000 --dump --trace 50
```

### Step by step

#### 1. Assemble service modules

```bash
cor24-run --assemble demos/multi/uart.s  build/uart.bin  build/uart.lst  --base-addr 0x1000
cor24-run --assemble demos/multi/spi.s   build/spi.bin   build/spi.lst   --base-addr 0x2000
cor24-run --assemble demos/multi/i2c.s   build/i2c.bin   build/i2c.lst   --base-addr 0x3000
cor24-run --assemble demos/multi/gpio.s  build/gpio.bin  build/gpio.lst  --base-addr 0x4000
cor24-run --assemble demos/multi/timer.s build/timer.bin build/timer.lst --base-addr 0x5000
```

Check a listing to see entry point addresses:

```bash
cat build/uart.lst
```

```
                    _uart_putc:
1000: 80             push    fp
1001: 7F             push    r2
...
```

#### 2. Compile the main program

```bash
just compile demos/multi/main.l24
```

#### 3. Run with all modules loaded

```bash
cor24-run --run build/main.s --load-binary build/uart.bin@0x1000 --load-binary build/spi.bin@0x2000 --load-binary build/i2c.bin@0x3000 --load-binary build/gpio.bin@0x4000 --load-binary build/timer.bin@0x5000 --speed 0 -n 10000000
```

## How It Works

### Service modules

Each module is a handwritten .s file following the COR24 ABI.
The entry point is at the first byte of the module. Example
(`demos/multi/uart.s`):

```asm
_uart_putc:
        push    fp
        push    r2
        push    r1
        mov     fp,sp
        lw      r0,9(fp)       ; arg: tagged fixnum
        lc      r1,2
        sra     r0,r1          ; untag
        la      r1,#xFF0100    ; UART data register
        sb      r0,0(r1)       ; write byte
        lw      r0,9(fp)       ; return the char (tagged)
        mov     sp,fp
        pop     r1
        pop     r2
        pop     fp
        jmp     (r1)
```

The stub modules (spi, i2c, gpio, timer) each print their name
by calling uart-putc at 0x1000, demonstrating cross-module calls
between service modules too.

### ABI convention

- Arguments are tagged fixnums (shifted left 2 bits)
- The callee must untag (`sra r0,r1` where r1=2) before using raw values
- Return value in r0 (tagged)
- Standard prologue: push fp, push r2, push r1, mov fp,sp
- Standard epilogue: mov sp,fp, pop r1, pop r2, pop fp, jmp (r1)
- First arg at fp+9, second at fp+12, etc.

### Main program

The main program (`demos/multi/main.l24`) is compiled Lisp that
maps names to addresses and calls each module:

```scheme
(define uart-putc  (asm "        la   r0,#x1000"))
(define spi-xfer   (asm "        la   r0,#x2000"))
(define i2c-xfer   (asm "        la   r0,#x3000"))
(define gpio-op    (asm "        la   r0,#x4000"))
(define timer-op   (asm "        la   r0,#x5000"))

(spi-xfer)
(i2c-xfer)
(gpio-op)
(timer-op)

(asm "_halt:"
     "        bra _halt")
```

The `(asm "        la   r0,#x1000")` inside `define` loads the
module's entry address into r0. The compiler stores this as the
global's value. When `(spi-xfer)` is called, the compiler emits
`la r0,<global>` / `lw r0,0(r0)` / `jal r1,(r0)` — a standard
function call to address 0x2000.

### Memory map

```
0x000000  main.s      compiled Lisp (_cmain entry, runs first)
   ...
0x001000  uart.bin    UART putc service
0x002000  spi.bin     SPI stub (prints "SPI")
0x003000  i2c.bin     I2C stub (prints "I2C")
0x004000  gpio.bin    GPIO stub (prints "GPIO")
0x005000  timer.bin   Timer stub (prints "TMR")
   ...
0xFEE000  Stack       8 KB EBR, grows downward
0xFF0000  I/O         LED, button, UART, interrupt enable
```

## Writing Your Own Modules

### Service module (.s file)

Write a .s file with the entry point at the first label:

```asm
; mymodule.s
_my_function:
        push    fp
        push    r2
        push    r1
        mov     fp,sp
        ; ... your code, args at fp+9, fp+12, ...
        ; ... result in r0 (tagged fixnum) ...
        mov     sp,fp
        pop     r1
        pop     r2
        pop     fp
        jmp     (r1)
```

Assemble at the target address:

```bash
cor24-run --assemble mymodule.s build/mymodule.bin build/mymodule.lst --base-addr 0x6000
```

### Multiple entry points

Use a jump table at the module base:

```asm
; Each entry: la (4 bytes) + jmp (1 byte) = 5 bytes
; Entry 0 at base+0, entry 1 at base+5, entry 2 at base+10
        la      r0,_func_a
        jmp     (r0)
        la      r0,_func_b
        jmp     (r0)

_func_a:
        ; ... standard ABI ...
        jmp     (r1)

_func_b:
        ; ... standard ABI ...
        jmp     (r1)
```

Reference from Lisp:

```scheme
(define func-a (asm "        la   r0,#x6000"))  ; base + 0
(define func-b (asm "        la   r0,#x6005"))  ; base + 5
```

### Cross-module calls from .s

Service modules can call other modules by address. All stubs in
the demo call uart-putc at 0x1000 to print their name:

```asm
        la      r0,332         ; 'S' tagged (83*4)
        push    r0
        la      r0,#x1000     ; uart-putc
        jal     r1,(r0)
        add     sp,3
```

## Limitations

- **Hardcoded addresses**: the main program must know each module's
  base address at compile time. No dynamic linking.
- **No symbol sharing**: modules can't reference each other's labels
  by name. Cross-module calls use fixed addresses.
- **Manual coordination**: you choose base addresses and ensure modules
  don't overlap.
- **Service modules are handwritten .s**: the compiler wraps everything
  in a `_cmain` prologue, so compiled .l24 files can't serve as
  loadable modules directly.

## Future: Simple Linker

A build step could automate address assignment and cross-module
references:

```bash
just link main.l24 uart.l24 spi.l24
```

This would:
1. Compile each .l24 to .s
2. Assign base addresses (avoiding overlaps)
3. Extract exported labels from .lst files
4. Patch cross-references
5. Produce coordinated .bin files or one merged binary
