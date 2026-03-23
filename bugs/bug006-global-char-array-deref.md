# Bug 006: Global char array treated as pointer instead of array

## Summary

When indexing into a global `char` array (e.g., `name_pool[0] = 'H'`), the compiler generates code that dereferences the array's address as if it were a pointer variable, rather than using the array address directly.

## Minimal reproducer

```c
// bug006.c
void putc_uart(int ch) {
    while (*(char *)0xFF0101 & 0x80) {}
    *(char *)0xFF0100 = ch;
}

int big[4096];
char pool[100];

int main() {
    pool[0] = 65;
    pool[1] = 66;
    pool[2] = 0;
    putc_uart(pool[0]);
    putc_uart(pool[1]);
    putc_uart(10);
    return 0;
}
```

Wait — this works when `big` is a single array. The bug only manifests with multiple large arrays preceding the char array. Actual reproducer:

```c
// bug006.c
void putc_uart(int ch) {
    while (*(char *)0xFF0101 & 0x80) {}
    *(char *)0xFF0100 = ch;
}

int arr1[4096];
int arr2[4096];
char pool[100];

int main() {
    pool[0] = 65;
    pool[1] = 0;
    putc_uart(pool[0]);
    putc_uart(10);
    return 0;
}
```

## Command

```
tc24r bug006.c -o bug006.s
cor24-run --run bug006.s
```

## Actual output

Empty — `pool[0]` reads back as 0.

## Expected output

```
A
```

## Generated assembly (wrong)

For `pool[0] = 65`:
```asm
        la      r1,_pool
        lw      r0,0(r1)     ; BUG: loads *pool (memory contents) as a word
        lc      r1,0          ; index 0
        add     r0,r1         ; adds index to garbage value
        ...
        sb      r0,0(r1)     ; stores to wrong address
```

## Expected assembly

```asm
        la      r0,_pool     ; load ADDRESS of pool
        lc      r1,65        ; 'A'
        sb      r1,0(r0)     ; store byte at pool[0]
```

## Root cause

The codegen treats the global char array as a pointer variable (loads its contents with `lw`) instead of as an array (uses its address directly with `la`). This may be specific to char arrays following large int arrays, or may affect all global char arrays in certain contexts.

## Impact

Cannot use global char arrays for string storage. Blocks symbol interning in tml24c.
