# Bug 001: Nested function-like macro not expanded

## Summary

When a `#define` constant references a function-like macro, the preprocessor does not expand the inner macro. Instead, the compiler treats it as a function call.

## Minimal reproducer

```c
// bug001.c
#define MAKE_SYMBOL(idx) (((idx) << 2) | 2)
#define NIL_VAL MAKE_SYMBOL(0)

int main() {
    int x = NIL_VAL;
    return x;
}
```

## Command

```
tc24r bug001.c -o bug001.s
```

## Actual output

Compiles without error, but generated assembly contains:

```asm
la      r0,_MAKE_SYMBOL
jal     r1,(r0)
```

The compiler treats `MAKE_SYMBOL(0)` as a function call to `_MAKE_SYMBOL` instead of expanding the macro.

## Expected output

`MAKE_SYMBOL(0)` should expand to `(((0) << 2) | 2)` which is the constant `2`. The generated assembly should load the immediate value `2`, not call a function.

## Impact

Cannot use layered `#define` macros where an object-like macro references a function-like macro. This is standard C preprocessor behavior (C99 §6.10.3.4) — macro replacement is recursive.

## Workaround

Manually inline the expansion:

```c
// Instead of:
#define NIL_VAL MAKE_SYMBOL(0)
// Use:
#define NIL_VAL 2    // ((0 << 2) | 2)
```
