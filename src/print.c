#include "tml.h"

static void print_string(const char *s) {
    while (*s) {
        platform_putchar(*s++);
    }
}

static void print_int(int32_t n) {
    if (n < 0) {
        platform_putchar('-');
        /* Handle INT32_MIN carefully */
        if (n == -2147483647 - 1) {
            print_string("2147483648");
            return;
        }
        n = -n;
    }
    if (n >= 10) {
        print_int(n / 10);
    }
    platform_putchar('0' + (int)(n % 10));
}

static void print_list(Value v) {
    platform_putchar('(');
    int first = 1;
    while (IS_CONS(v)) {
        if (!first) platform_putchar(' ');
        first = 0;
        print_val(CAR(v));
        v = CDR(v);
    }
    if (!IS_NIL(v)) {
        print_string(" . ");
        print_val(v);
    }
    platform_putchar(')');
}

void print_val(Value v) {
    if (IS_FIXNUM(v)) {
        print_int(FIXNUM_VAL(v));
    } else if (IS_NIL(v)) {
        print_string("nil");
    } else if (IS_SYMBOL(v)) {
        print_string(symbol_name(v));
    } else if (IS_CONS(v)) {
        print_list(v);
    } else if (IS_EXTENDED(v)) {
        print_string("#<object>");
    }
}
