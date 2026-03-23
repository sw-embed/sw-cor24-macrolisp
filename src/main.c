#include "tml.h"

static void print_string(const char *s) {
    while (*s) {
        platform_putchar(*s++);
    }
}

static void test_scaffold(void) {
    /* Test fixnum */
    Value n = MAKE_FIXNUM(42);
    print_string("fixnum 42 = ");
    print_val(n);
    platform_putchar('\n');

    /* Test symbols */
    Value nil = intern("nil");
    Value t = intern("t");
    Value foo = intern("foo");
    print_string("nil = ");
    print_val(nil);
    platform_putchar('\n');
    print_string("t = ");
    print_val(t);
    platform_putchar('\n');
    print_string("foo = ");
    print_val(foo);
    platform_putchar('\n');

    /* Test cons */
    Value pair = cons(MAKE_FIXNUM(1), MAKE_FIXNUM(2));
    print_string("(1 . 2) = ");
    print_val(pair);
    platform_putchar('\n');

    /* Test list: (a b c) */
    Value a = intern("a");
    Value b = intern("b");
    Value c = intern("c");
    Value lst = cons(a, cons(b, cons(c, NIL_VAL)));
    print_string("(a b c) = ");
    print_val(lst);
    platform_putchar('\n');

    print_string("tml24c ok\n");
}

int main(void) {
    heap_init();
    symbol_init();
    test_scaffold();
    return 0;
}
