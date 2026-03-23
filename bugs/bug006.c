// Bug 006: Global char array treated as pointer instead of array
// pool[0] = 65 generates lw (dereference) instead of la (address-of)

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
    putc_uart(pool[0]);  // expect 'A'
    putc_uart(10);
    return 0;
}
