# COR24 ISA Summary for Lisp Implementation

## Machine Overview

- **Word size:** 24-bit (3 bytes)
- **Endianness:** Little-endian
- **Address space:** 24-bit (16 MB)
- **Instruction encoding:** Variable-length (1, 2, or 4 bytes)

## Registers (8 total, 24-bit each)

| Register | Alias | Role | Saved By |
|----------|-------|------|----------|
| r0 | — | Return value / scratch | Caller |
| r1 | — | Return address (link register) | Callee |
| r2 | — | General purpose | Callee |
| r3 | fp | Frame pointer | Callee |
| r4 | sp | Stack pointer (grows down) | — |
| r5 | z | Always zero (read-only) | — |
| r6 | iv | Interrupt vector | — |
| r7 | ir | Interrupt return address | — |

**Implication for Lisp:** Only 3 general-purpose registers (r0, r1, r2). Heavy stack usage required for expression evaluation and argument passing. Interpreter dispatch will be call-heavy.

## Memory Map

| Range | Size | Use |
|-------|------|-----|
| 0x000000–0x0FFFFF | 1 MB | SRAM (main memory) |
| 0x100000–0xFDFFFF | ~13 MB | Unmapped |
| 0xFEE000–0xFEFFFF | 3 KB (populated) | Embedded Block RAM (fast) |
| 0xFF0000–0xFFFFFF | — | Memory-mapped I/O |

**Stack:** Initialized at 0xFEEC00 (top of EBR), grows downward.

**Heap for Lisp:** Will live in SRAM (0x000000–0x0FFFFF). Code also in SRAM. Need to partition: code at low addresses, heap growing up, stack at EBR growing down.

## I/O Registers

| Address | Name | Description |
|---------|------|-------------|
| 0xFF0000 | LED_DATA | LED D2 (write bit 0, active low) / Button S2 (read bit 0) |
| 0xFF0010 | INT_ENABLE | UART RX interrupt enable (bit 0) |
| 0xFF0100 | UART_DATA | TX: write byte / RX: read byte (auto-ack) |
| 0xFF0101 | UART_STAT | bit 0: RX ready, bit 1: CTS, bit 2: overflow, bit 7: TX busy |

**UART protocol:** Poll UART_STAT bit 7 before TX write; poll bit 0 before RX read.

## Instruction Set (32 operations)

### Arithmetic
| Instruction | Bytes | Operation |
|-------------|-------|-----------|
| `add ra,rb` | 1 | ra = ra + rb |
| `add ra,dd` | 2 | ra = ra + sign_extend(dd) |
| `sub ra,rb` | 1 | ra = ra - rb |
| `sub sp,dddddd` | 4 | sp = sp - imm24 |
| `mul ra,rb` | 1 | ra = ra * rb (lower 24 bits) |

**No hardware divide.** Division/modulo require runtime helper functions (repeated subtraction).

### Logical & Shift
| Instruction | Bytes | Operation |
|-------------|-------|-----------|
| `and ra,rb` | 1 | ra = ra & rb |
| `or ra,rb` | 1 | ra = ra \| rb |
| `xor ra,rb` | 1 | ra = ra ^ rb |
| `shl ra,rb` | 1 | ra = ra << rb[4:0] |
| `sra ra,rb` | 1 | ra = ra >> rb[4:0] (arithmetic) |
| `srl ra,rb` | 1 | ra = ra >> rb[4:0] (logical) |

### Compare (sets condition flag C)
| Instruction | Bytes | Operation |
|-------------|-------|-----------|
| `ceq ra,rb` | 1 | C = (ra == rb) |
| `cls ra,rb` | 1 | C = (ra < rb) signed |
| `clu ra,rb` | 1 | C = (ra < rb) unsigned |

### Branch
| Instruction | Bytes | Operation |
|-------------|-------|-----------|
| `bra dd` | 2 | PC = PC + 4 + sign_extend(dd) |
| `brt dd` | 2 | if (C) branch |
| `brf dd` | 2 | if (!C) branch |

Branch range: ±127 bytes (8-bit signed offset). Long jumps use `la` + `jmp`.

### Jump & Call
| Instruction | Bytes | Operation |
|-------------|-------|-----------|
| `jmp (ra)` | 1 | PC = ra |
| `jal ra,(rb)` | 1 | ra = PC+1; PC = rb |

### Load
| Instruction | Bytes | Operation |
|-------------|-------|-----------|
| `la ra,dddddd` | 4 | ra = imm24 |
| `lc ra,dd` | 2 | ra = sign_extend(dd) |
| `lcu ra,dd` | 2 | ra = zero_extend(dd) |
| `lw ra,dd(rb)` | 2 | ra = mem24[rb + sign_extend(dd)] |
| `lb ra,dd(rb)` | 2 | ra = sign_extend(mem8[rb + dd]) |
| `lbu ra,dd(rb)` | 2 | ra = zero_extend(mem8[rb + dd]) |

### Store
| Instruction | Bytes | Operation |
|-------------|-------|-----------|
| `sw ra,dd(rb)` | 2 | mem24[rb + dd] = ra |
| `sb ra,dd(rb)` | 2 | mem8[rb + dd] = ra[7:0] |

### Stack
| Instruction | Bytes | Operation |
|-------------|-------|-----------|
| `push ra` | 1 | sp -= 3; mem24[sp] = ra |
| `pop ra` | 1 | ra = mem24[sp]; sp += 3 |

### Move & Extend
| Instruction | Bytes | Operation |
|-------------|-------|-----------|
| `mov ra,rb` | 1 | ra = rb |
| `sxt ra,rb` | 1 | ra = sign_extend(rb[7:0]) |
| `zxt ra,rb` | 1 | ra = zero_extend(rb[7:0]) |

## Operations Native vs Needing Runtime Support

### Native (single instruction)
- Integer add, subtract, multiply
- Bitwise AND, OR, XOR
- Shifts (logical, arithmetic)
- Compare (eq, signed-less, unsigned-less)
- Conditional branch
- Word/byte load/store
- Push/pop (3-byte words)
- Function call/return (jal/jmp)

### Needs Runtime Support
- **Integer divide/modulo** — repeated subtraction loop
- **String operations** — byte-by-byte loops
- **Memory copy/fill** — no block move instruction
- **Multi-word arithmetic** — if fixnum range exceeded
- **Printf/formatting** — character-by-character UART output
- **Heap allocation** — bump pointer or free-list in software
- **Garbage collection** — entirely in software

## Calling Convention (tc24r ABI)

- **Arguments:** Pushed right-to-left on stack, caller cleans up
- **Return value:** r0
- **Callee-saved:** r1, r2, fp
- **Frame:** push fp; push r2; push r1; mov fp,sp
- **Locals:** Negative offsets from fp (fp-3, fp-6, ...)
- **Args:** Positive offsets from fp (fp+9, fp+12, ...)
- **Halt:** `bra halt` (self-branch detected by emulator)

## Data Types (tc24r)

| C Type | Size | Notes |
|--------|------|-------|
| char | 1 byte | Signed, sign-extended on load |
| int | 3 bytes | Native machine word |
| long | 3 bytes | Same as int |
| pointer | 3 bytes | Full 24-bit address |
| enum | 3 bytes | Same as int |

**Integer range:** -8,388,608 to 8,388,607 (signed 24-bit)

## Implications for Lisp Tagged Values

With 24-bit words, a tagged-pointer scheme might use:
- **Low 2 bits as tag** (4 tag values, pointers 3-byte aligned)
- **Low 3 bits as tag** (8 tag values, but wastes more pointer bits)

Possible 2-bit scheme:
| Tag | Meaning |
|-----|---------|
| 00 | Fixnum (22-bit signed integer, shifted right 2) |
| 01 | Cons pair pointer |
| 10 | Symbol pointer |
| 11 | Other heap object pointer (string, closure, etc.) |

22-bit fixnum range: -2,097,152 to 2,097,151 — adequate for demos.

## Assembler Syntax

```asm
; Comments start with semicolon
label:
        lc      r0,42           ; 8-bit signed immediate
        la      r1,0xFF0100     ; 24-bit immediate
        add     r0,r1           ; register-register
        lw      r0,-3(fp)       ; load word with offset
        push    r0              ; stack push
        jal     r1,(r0)         ; call through r0
        bra     label           ; branch to label
```

**Directives:** `.org`, `.byte`, `.word`, `.comm`, `.globl`

**Binary format:** LGO (Load-and-Go) — `L<addr><hex bytes>` lines + `G<addr>` entry point.

## Emulator Capabilities

- Step, run, breakpoint, continue
- UART I/O capture (`get_uart_output()`)
- Memory/register inspection
- Trace buffer (last 200 instructions)
- Halt detection (self-branch)
- CLI debugger: `cor24-dbg <file.lgo>`
