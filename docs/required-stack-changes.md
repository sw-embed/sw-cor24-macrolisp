# Stack Configuration in cor24-emu

## Background

COR24 has two memory regions usable for the stack:

| Region | Address Range | Size | Speed | Notes |
|--------|-------------|------|-------|-------|
| EBR | 0xFEE000–0xFEFFFF | 8 KB | Fast (single-cycle) | Embedded block RAM |
| SRAM | 0x000000–0x0FFFFF | 1 MB | Slower | Shared with code and data |

The default COR24-TB configuration uses EBR for the stack with SP initialized at 0xFEEC00, giving ~3KB usable (growing down to 0xFEE000).

## `--stack-kilobytes` flag

```
cor24-emu --lgo <file.lgo> --stack-kilobytes 3    # default (EBR, COR24-TB)
cor24-emu --lgo <file.lgo> --stack-kilobytes 8    # full EBR (max for this flag)
```

`cor24-emu --stack-kilobytes` accepts only `3` or `8` (EBR), and **8 KB EBR is
the hard ceiling** — the emulator enforces the stack inside the EBR window and
halts with "Stack overflow: SP=… below stack base" if SP is set anywhere else
(including the top of SRAM). The `eval-fullbig` recipe runs the full prelude at
this 8 KB maximum; use it when a program overflows the 3 KB default.

> A larger SRAM-backed stack (SP relocated to the top of the 1 MB SRAM) is a
> legitimate convention the hardware allows, but the current `cor24-emu` has no
> SRAM-aware stack mode — its overflow/underflow guard is pinned to EBR. That's
> a potential emulator enhancement (dcemu), not something this repo can do. No
> current demo needs more than 8 KB, so it is not required.

### Stack placement

| Size | Initial SP | Region | Use case |
|------|-----------|--------|----------|
| 3 KB | 0xFEEC00 | EBR | Default COR24-TB, standard prelude |
| 8 KB | 0xFF0000 | Full EBR | Full prelude, complex programs |
| 16+ KB | SRAM top-down | SRAM | Experimental, deep recursion |

### SRAM stack (>8KB) — design note, not currently usable on cor24-emu

> **Status:** `cor24-emu` does not implement an SRAM stack mode; its stack guard
> rejects any SP outside the EBR window. The notes below describe the intended
> design (and what the FPGA hardware permits) for when the emulator gains the
> capability. Until then, 8 KB EBR (`--stack-kilobytes 8`) is the effective max.

For stacks larger than 8KB, the stack can be placed at the top of SRAM (growing down from 0x0FFFFF or a configured boundary). This uses SRAM instead of EBR:

- **Pros**: Up to ~100KB+ of stack space, limited only by code/data size
- **Cons**: Slower access (SRAM is not single-cycle on all COR24 variants), shares space with heap arrays and code
- **Layout**: Code at low addresses, heap arrays above code, stack at top of SRAM growing down. Need to ensure stack doesn't collide with heap.

For tml24c, SRAM stack would require adjusting `gc_initial_sp` capture — the conservative GC scanner must know the stack bounds. Since `gc_init()` captures SP via inline asm at startup, it automatically adapts to wherever SP starts.

### Typical stack requirements

| Prelude | Estimated stack | Recommended flag |
|---------|----------------|------------------|
| Tiny (primitives only) | ~1 KB | `--stack-kilobytes 3` (default) |
| Standard (core Lisp) | ~2 KB | `--stack-kilobytes 3` |
| Full (lazy, threading, etc.) | ~4 KB | `--stack-kilobytes 8` |
| Experimental | ~4–6 KB | `--stack-kilobytes 8` |
| Deep recursion / stress test | up to 8 KB | `--stack-kilobytes 8` (`just eval-fullbig`) |

### Impact on tml24c

No tml24c code changes needed. The conservative GC captures the actual SP at startup and scans from current SP to initial SP, regardless of where the stack is placed.

The justfile would use:
```
run:      cor24-emu --lgo build/repl-standard.lgo --terminal --echo --speed 0
run-full: cor24-emu --lgo build/repl-full.lgo --terminal --echo --speed 0 --stack-kilobytes 8
```
