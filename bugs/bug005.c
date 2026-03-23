// Bug 005: Global arrays allocated as single word regardless of declared size
// int arr[100] emits .word 0 (3 bytes) instead of 300 bytes

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
