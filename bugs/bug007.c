// Bug 007: Array store with global-derived index always writes to index 0
// offsets[idx] = counter where idx = counter doesn't work

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
    do_store();
    do_store();
    do_store();
    print_int(offsets[0]); putc_uart(32);
    print_int(offsets[1]); putc_uart(32);
    print_int(offsets[2]); putc_uart(10);
    return 0;
}
