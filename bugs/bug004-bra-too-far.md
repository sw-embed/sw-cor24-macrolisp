# Bug 004: Unconditional branch (bra) generated for far target

## Summary

tc24r generates `bra L36` for a branch target ~90 lines (>127 bytes) away. The assembler rejects it because `bra` has an 8-bit signed offset (±127 bytes). tc24r should emit a long branch sequence (`la r7,L36` / `jmp (r7)` or equivalent) for far targets.

## Context

This occurs when compiling tml24c (`src/main.c` with includes). The generated assembly has:
- Line 574: `bra L36`
- Line 665: `L36:` label definition

The branch spans ~91 lines of instructions, well beyond the ±127 byte range.

## Reproducer

```
tc24r src/main.c -o build/tml24c.s
cor24-run --run build/tml24c.s
```

Output:
```
Assembly errors:
  Line 574: Branch target 'L36' too far
```

## Expected

tc24r should detect that L36 is too far for a short `bra` and emit a long jump instead.

## Generated assembly (relevant section)

```
build/tml24c.s line 574:  bra     L36
build/tml24c.s line 665:  L36:
```
