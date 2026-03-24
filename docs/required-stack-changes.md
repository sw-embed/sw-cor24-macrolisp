# Required Stack Changes in cor24-rs

## Problem

COR24-TB has 8KB EBR (0xFEE000–0xFEFFFF) with SP initialized at 0xFEEC00, giving ~3KB of usable stack (growing down to 0xFEE000). This is tight for tml24c with:

- Prelude loading: ~60 definitions, each requiring eval + GC overhead
- GC mark phase: recursive marking of deep data structures
- Complex Lisp programs: deep call chains, lazy sequences, string operations

## Proposed: `--stack` flag for cor24-run

```
cor24-run --run <file.s> --stack small    # 3KB (current default, COR24-TB)
cor24-run --run <file.s> --stack large    # 8KB (full EBR)
cor24-run --run <file.s> --stack 4096     # custom byte count
```

### Implementation

Change the initial SP based on the flag:

| Stack Size | Initial SP | Usable Range | Notes |
|-----------|-----------|-------------|-------|
| `small` (3KB) | 0xFEEC00 | 0xFEE000–0xFEEC00 | Current COR24-TB default |
| `large` (8KB) | 0xFF0000 | 0xFEE000–0xFF0000 | Uses all of EBR |
| Custom | Computed | 0xFEE000 + N | User-specified bytes |

The `large` option sets SP to the top of EBR (0xFF0000, which is also the start of I/O space — SP decrements before writing, so the first push goes to 0xFEFFFF).

### Impact on tml24c

| Prelude | Stack needed | `--stack` setting |
|---------|-------------|-------------------|
| Tiny | ~1KB | `small` (default) |
| Standard | ~2KB | `small` |
| Full | ~4KB | `large` |
| Experimental | ~4KB+ | `large` |

### Changes needed in cor24-rs

1. Add `--stack` CLI flag to `CliArgs` in `run.rs`
2. After loading program, set SP: `emu.set_sp(initial_sp)`
3. Update conservative GC: `gc_initial_sp` must match the actual initial SP
4. tml24c's `gc_init()` captures SP at startup, so it automatically adapts

### Verification

The tml24c conservative GC captures initial SP via inline asm at startup. As long as cor24-run sets SP before execution begins, no tml24c changes are needed.
