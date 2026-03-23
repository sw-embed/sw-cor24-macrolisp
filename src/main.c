/* tml24c -- Tiny Macro Lisp for COR24 */

#include "tml.h"
#include "io.h"
#include "heap.h"
#include "symbol.h"
#include "print.h"

void test_scaffold() {
    /* Fixnum */
    puts_str("fixnum 42 = ");
    print_val(MAKE_FIXNUM(42));
    putc_uart(10);

    /* Symbols */
    puts_str("nil = ");
    print_val(NIL_VAL);
    putc_uart(10);

    puts_str("t = ");
    print_val(T_VAL);
    putc_uart(10);

    int foo = intern("foo");
    puts_str("foo = ");
    print_val(foo);
    putc_uart(10);

    /* Cons pair */
    int pair = cons(MAKE_FIXNUM(1), MAKE_FIXNUM(2));
    puts_str("(1 . 2) = ");
    print_val(pair);
    putc_uart(10);

    /* List (a b c) */
    int sa = intern("a");
    int sb = intern("b");
    int sc = intern("c");
    int lst = cons(sa, cons(sb, cons(sc, NIL_VAL)));
    puts_str("(a b c) = ");
    print_val(lst);
    putc_uart(10);

    /* Nested list (1 (2 3)) */
    int inner = cons(MAKE_FIXNUM(2), cons(MAKE_FIXNUM(3), NIL_VAL));
    int nested = cons(MAKE_FIXNUM(1), cons(inner, NIL_VAL));
    puts_str("(1 (2 3)) = ");
    print_val(nested);
    putc_uart(10);

    puts_str("tml24c ok\n");
}

int main() {
    heap_init();
    symbol_init();
    test_scaffold();
    return 0;
}
