# Bug 005: Global arrays allocated as single word regardless of declared size

## Summary

Global array declarations like `int arr[4096]` are emitted as `.word 0` (3 bytes) in the `.data` section instead of allocating the full array size. All global arrays end up overlapping at nearly the same address.

## Minimal reproducer

```c
// bug005.c
int arr[100];

void putc_uart(int ch) {
    while (*(char *)0xFF0101 & 0x80) {}
    *(char *)0xFF0100 = ch;
}

int main() {
    arr[0] = 65;
    arr[99] = 66;
    putc_uart(arr[0]);   // expect 'A' (65)
    putc_uart(arr[99]);  // expect 'B' (66)
    putc_uart(10);
    return 0;
}
```

## Command

```
tc24r bug005.c -o bug005.s
```

## Actual generated .data section

```asm
        .data
_arr:
        .word   0
```

Only 3 bytes allocated for a 300-byte array.

## Expected .data section

```asm
        .data
_arr:
        .comm   arr,300     ; or 100 .word directives
```

Should allocate 100 × 3 = 300 bytes.

## Impact

All global arrays overlap in memory. Writes to one array corrupt others. This is a critical bug for any program using multiple global arrays.

## Observed in

tml24c scaffold: `int heap_car[4096]`, `int heap_cdr[4096]`, `char name_pool[2048]`, `int sym_name_off[256]` all overlap.
