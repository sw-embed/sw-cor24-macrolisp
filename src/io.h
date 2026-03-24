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

int getc_uart() {
    while (!(*(char *)UART_STATUS & 0x01)) {}  /* bit 0: RX data ready */
    return *(char *)UART_DATA;
}

void halt() {
    asm("_user_halt:");
    asm("bra _user_halt");
}

int read_line(char *buf, int max) {
    int i = 0;
    while (i < max - 1) {
        int ch = getc_uart();
        if (ch == '\n' || ch == '\r') {
            break;
        }
        if (ch == 4) {
            /* Ctrl-D: EOF */
            return -1;
        }
        buf[i] = ch;
        i = i + 1;
    }
    buf[i] = 0;
    return i;
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
