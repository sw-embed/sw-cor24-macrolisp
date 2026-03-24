/* tml24c -- Tiny Macro Lisp for COR24 */

#include "tml.h"
#include "io.h"
#include "heap.h"
#include "symbol.h"
#include "print.h"
#include "read.h"
#include "eval.h"
#include "gc.h"

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

    puts_str("scaffold ok\n");
}

void test_read_one(char *input, char *expected) {
    int val = read_str(input);
    puts_str(input);
    puts_str(" => ");
    print_val(val);
    puts_str(" [");
    puts_str(expected);
    puts_str("]\n");
}

void test_reader() {
    /* Integers */
    test_read_one("42", "42");
    test_read_one("-7", "-7");
    test_read_one("0", "0");

    /* Symbols */
    test_read_one("foo", "foo");
    test_read_one("nil", "nil");
    test_read_one("+", "+");

    /* Empty list */
    test_read_one("()", "nil");

    /* Simple list */
    test_read_one("(1 2 3)", "(1 2 3)");

    /* Dotted pair */
    test_read_one("(1 . 2)", "(1 . 2)");

    /* Nested list */
    test_read_one("(a (b c))", "(a (b c))");

    /* Quote */
    test_read_one("'x", "(quote x)");

    /* Mixed */
    test_read_one("(+ 1 2)", "(+ 1 2)");
    test_read_one("(define x 42)", "(define x 42)");

    puts_str("reader ok\n");
}

void test_eval_one(char *input, char *expected) {
    int expr = read_str(input);
    int result = eval(expr, NIL_VAL);
    puts_str(input);
    puts_str(" => ");
    print_val(result);
    puts_str(" [");
    puts_str(expected);
    puts_str("]\n");
}

void test_eval() {
    /* Self-evaluating */
    test_eval_one("42", "42");
    test_eval_one("-7", "-7");

    /* Quote */
    test_eval_one("(quote foo)", "foo");
    test_eval_one("'(1 2 3)", "(1 2 3)");

    /* Arithmetic */
    test_eval_one("(+ 1 2)", "3");
    test_eval_one("(- 10 3)", "7");
    test_eval_one("(* 4 5)", "20");
    test_eval_one("(/ 10 3)", "3");

    /* Nested arithmetic */
    test_eval_one("(+ (* 2 3) (- 10 4))", "12");

    /* Comparison */
    test_eval_one("(< 1 2)", "t");
    test_eval_one("(< 2 1)", "nil");
    test_eval_one("(= 5 5)", "t");
    test_eval_one("(= 5 6)", "nil");

    /* If */
    test_eval_one("(if t 1 2)", "1");
    test_eval_one("(if nil 1 2)", "2");
    test_eval_one("(if (< 1 2) 10 20)", "10");

    /* List operations */
    test_eval_one("(cons 1 2)", "(1 . 2)");
    test_eval_one("(car (cons 1 2))", "1");
    test_eval_one("(cdr (cons 1 2))", "2");
    test_eval_one("(list 1 2 3)", "(1 2 3)");

    /* Predicates */
    test_eval_one("(null? nil)", "t");
    test_eval_one("(null? 1)", "nil");
    test_eval_one("(pair? (cons 1 2))", "t");
    test_eval_one("(atom? 42)", "t");
    test_eval_one("(not nil)", "t");
    test_eval_one("(not t)", "nil");

    /* Lambda */
    test_eval_one("((lambda (x) (+ x 1)) 41)", "42");
    test_eval_one("((lambda (x y) (+ x y)) 3 4)", "7");

    /* Begin */
    test_eval_one("(begin 1 2 3)", "3");

    /* Define and use */
    eval(read_str("(define x 42)"), NIL_VAL);
    test_eval_one("x", "42");

    eval(read_str("(define double (lambda (n) (* n 2)))"), NIL_VAL);
    test_eval_one("(double 21)", "42");

    /* Higher-order */
    eval(read_str("(define apply-twice (lambda (f x) (f (f x))))"), NIL_VAL);
    test_eval_one("(apply-twice double 5)", "20");

    /* Closure */
    eval(read_str("(define make-adder (lambda (n) (lambda (x) (+ n x))))"), NIL_VAL);
    eval(read_str("(define add5 (make-adder 5))"), NIL_VAL);
    test_eval_one("(add5 10)", "15");

    /* Recursion */
    eval(read_str("(define fact (lambda (n) (if (= n 0) 1 (* n (fact (- n 1))))))"), NIL_VAL);
    test_eval_one("(fact 5)", "120");

    puts_str("eval ok\n");
}

void test_gc() {
    /* Record heap state before GC test */
    int before_next = heap_next;

    /* Allocate a bunch of garbage (unreachable cells) */
    int i = 0;
    while (i < 100) {
        cons(MAKE_FIXNUM(i), MAKE_FIXNUM(i));
        i = i + 1;
    }

    int after_alloc = heap_next;
    puts_str("allocated 100 garbage pairs, heap_next=");
    print_int(after_alloc);
    putc_uart(10);

    /* Force a collection */
    gc_collect();

    /* Free list should now have cells available */
    puts_str("gc collections=");
    print_int(gc_collections);
    putc_uart(10);

    /* Verify reachable data survived: eval a simple expression */
    test_eval_one("(+ 1 2)", "3");
    test_eval_one("x", "42");
    test_eval_one("(double 21)", "42");
    test_eval_one("(fact 5)", "120");

    /* Allocate more -- should reuse freed cells */
    int v1 = cons(MAKE_FIXNUM(99), NIL_VAL);
    gc_protect(v1);
    puts_str("post-gc cons = ");
    print_val(v1);
    putc_uart(10);

    /* Verify the protected value survives another GC */
    gc_collect();
    puts_str("after 2nd gc, val = ");
    print_val(v1);
    putc_uart(10);
    gc_unprotect(1);

    /* Final check: everything still works */
    test_eval_one("(+ 100 200)", "300");

    puts_str("gc ok\n");
}

int main() {
    heap_init();
    gc_init();
    symbol_init();
    eval_init();
    gc_enabled = 1;  /* enable GC after init */
    test_scaffold();
    test_reader();
    test_eval();
    test_gc();
    return 0;
}
