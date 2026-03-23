# Bug 002: Compiler panic on two-level #define constant expression

## Summary

When a `#define` object-like macro references another `#define` object-like macro, and the result is used in an expression (e.g., array assignment), the compiler panics with "no entry found for key" in `tc24r-emit-load-store`.

## Minimal reproducer

```c
// bug002.c
#define TAG_SYMBOL 2
#define NIL_VAL ((0 << 2) | TAG_SYMBOL)

int arr[10];

int main() {
    arr[0] = NIL_VAL;
    return arr[0];
}
```

## Command

```
tc24r bug002.c -o bug002.s
```

## Actual output

```
thread 'main' panicked at .../tc24r-emit-load-store/src/load.rs:13:34:
no entry found for key
```

## Expected output

Should compile successfully. `NIL_VAL` expands to `((0 << 2) | 2)` which is the constant `2`.

## Notes

- Single-level `#define` works: `#define NIL_VAL ((0 << 2) | 2)` compiles fine
- The issue is specifically when one object-like macro references another in an expression with operators

## Workaround

Manually compute and inline the constant:

```c
// Instead of:
#define TAG_SYMBOL 2
#define NIL_VAL ((0 << 2) | TAG_SYMBOL)
// Use:
#define NIL_VAL 2
```
