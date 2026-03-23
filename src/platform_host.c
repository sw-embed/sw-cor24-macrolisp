#include "tml.h"
#include <stdio.h>
#include <stdlib.h>

void platform_putchar(int ch) {
    putchar(ch);
}

int platform_getchar(void) {
    return getchar();
}

void platform_halt(int code) {
    exit(code);
}
