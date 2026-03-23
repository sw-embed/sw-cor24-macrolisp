#pragma once

/* io.h -- UART I/O for COR24-TB */

#define UART_DATA   0xFF0100
#define UART_STATUS 0xFF0101

void putc_uart(int ch) {
    while (*(char *)UART_STATUS & 0x80) {}
    *(char *)UART_DATA = ch;
}

void puts_str(char *s) {
    while (*s) {
        putc_uart(*s);
        s = s + 1;
    }
}

void print_int(int n) {
    if (n < 0) {
        putc_uart(45);
        n = 0 - n;
    }
    if (n == 0) {
        putc_uart(48);
        return;
    }
    char buf[8];
    int i = 0;
    while (n > 0) {
        buf[i] = 48 + n % 10;
        n = n / 10;
        i = i + 1;
    }
    while (i > 0) {
        i = i - 1;
        putc_uart(buf[i]);
    }
}
