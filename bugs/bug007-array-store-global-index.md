# Bug 007: Array store with global-derived index always writes to index 0

## Summary

When storing to a global array using an index derived from a global variable, the store always goes to index 0 regardless of the actual index value.

## Minimal reproducer

```c
// bug007.c
void putc_uart(int ch) {
    while (*(char *)0xFF0101 & 0x80) {}
    *(char *)0xFF0100 = ch;
}
void print_int(int n) {
    if (n < 0) { putc_uart(45); n = 0 - n; }
    if (n == 0) { putc_uart(48); return; }
    char buf[8];
    int i = 0;
    while (n > 0) { buf[i] = 48 + n % 10; n = n / 10; i = i + 1; }
    while (i > 0) { i = i - 1; putc_uart(buf[i]); }
}

int offsets[10];
int counter;

void do_store() {
    int idx = counter;
    offsets[idx] = counter;
    counter = counter + 1;
}

int main() {
    counter = 0;
    do_store();  // should set offsets[0] = 0
    do_store();  // should set offsets[1] = 1
    do_store();  // should set offsets[2] = 2
    print_int(offsets[0]); putc_uart(32);
    print_int(offsets[1]); putc_uart(32);
    print_int(offsets[2]); putc_uart(10);
    return 0;
}
```

## Command

```
tc24r bug007.c -o bug007.s
cor24-run --run bug007.s
```

## Actual output

```
0 0 0
```

All stores go to index 0. `offsets[1]` and `offsets[2]` are never written.

## Expected output

```
0 1 2
```

## Root cause hypothesis

In `do_store()`, `offsets[idx] = counter;` — the codegen evaluates the RHS (`counter`) but clobbers the register holding the LHS index (`idx`) before the store. Both `idx` and `counter` involve loading globals, and the two-register machine can't hold both simultaneously.

## Impact

Cannot store to global arrays using computed indices derived from global state. Blocks tml24c symbol interning.
