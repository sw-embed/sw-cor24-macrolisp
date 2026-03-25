/* tml24c -- Tiny Macro Lisp for COR24 */

#include "tml.h"
#include "io.h"
#include "heap.h"
#include "symbol.h"
#include "string.h"
#include "print.h"
#include "read.h"
#include "eval.h"
#include "gc.h"
#include "compile.h"

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

    /* catch/throw — basic non-local exit */
    test_eval_one("(catch 'done 42)", "42");
    test_eval_one("(catch 'done (throw 'done 99))", "99");
    test_eval_one("(catch 'done (begin 1 (throw 'done 77) 3))", "77");

    /* catch/throw — skips unreachable code after throw */
    test_eval_one("(catch 'x (+ 1 (throw 'x 50)))", "50");

    /* catch/throw — nested, inner throw */
    test_eval_one("(catch 'outer (catch 'inner (throw 'inner 10)))", "10");

    /* catch/throw — nested, throw to outer */
    test_eval_one("(catch 'outer (catch 'inner (throw 'outer 20)))", "20");

    /* catch/throw — body returns normally */
    test_eval_one("(catch 'tag (+ 3 4))", "7");

    /* catch/throw — throw from inside a lambda call */
    eval(read_str("(define bail (lambda () (throw 'escape 123)))"), NIL_VAL);
    test_eval_one("(catch 'escape (bail))", "123");

    /* call/ec — basic escape continuation */
    test_eval_one("(call/ec (lambda (return) 42))", "42");
    test_eval_one("(call/ec (lambda (return) (return 99) 0))", "99");

    /* call/ec — early return from for-each */
    eval(read_str("(define (find-first pred lst) (call/ec (lambda (return) (for-each (lambda (x) (if (pred x) (return x) nil)) lst) nil)))"), NIL_VAL);
    test_eval_one("(find-first (lambda (x) (= x 3)) '(1 2 3 4 5))", "3");
    test_eval_one("(find-first (lambda (x) (= x 9)) '(1 2 3))", "nil");

    /* Error handling — with-handler catches raised errors */
    test_eval_one("(with-handler (lambda (e) (+ e 1)) (lambda () (raise 41)))", "42");

    /* Error handling — normal return when no error */
    test_eval_one("(with-handler (lambda (e) 0) (lambda () (+ 10 20)))", "30");

    /* Error handling — nested handlers, inner catches */
    test_eval_one("(with-handler (lambda (e) 'outer) (lambda () (with-handler (lambda (e) 'inner) (lambda () (raise 'oops)))))", "inner");

    /* Error handling — safe division */
    eval(read_str("(define (safe-div a b) (if (= b 0) (raise 'div-by-zero) (/ a b)))"), NIL_VAL);
    test_eval_one("(with-handler (lambda (e) -1) (lambda () (safe-div 10 2)))", "5");
    test_eval_one("(with-handler (lambda (e) -1) (lambda () (safe-div 10 0)))", "-1");

    /* dynamic-wind — after runs on normal exit */
    eval(read_str("(define dw-log nil)"), NIL_VAL);
    eval(read_str("(dynamic-wind (lambda () (set! dw-log (cons 'before dw-log))) (lambda () (set! dw-log (cons 'during dw-log))) (lambda () (set! dw-log (cons 'after dw-log))))"), NIL_VAL);
    test_eval_one("dw-log", "(after during before)");

    /* dynamic-wind — after runs on throw */
    eval(read_str("(set! dw-log nil)"), NIL_VAL);
    eval(read_str("(catch 'bail (dynamic-wind (lambda () (set! dw-log (cons 'before dw-log))) (lambda () (throw 'bail 0)) (lambda () (set! dw-log (cons 'after dw-log)))))"), NIL_VAL);
    test_eval_one("dw-log", "(after before)");

    /* unwind-protect — cleanup on normal exit */
    eval(read_str("(define up-log nil)"), NIL_VAL);
    eval(read_str("(unwind-protect (set! up-log (cons 'body up-log)) (set! up-log (cons 'cleanup up-log)))"), NIL_VAL);
    test_eval_one("up-log", "(cleanup body)");

    /* unwind-protect — cleanup on throw */
    eval(read_str("(set! up-log nil)"), NIL_VAL);
    eval(read_str("(catch 'abort (unwind-protect (begin (set! up-log (cons 'body up-log)) (throw 'abort 0)) (set! up-log (cons 'cleanup up-log))))"), NIL_VAL);
    test_eval_one("up-log", "(cleanup body)");

    /* guard — basic clause matching */
    test_eval_one("(guard (e ((eq? e 'div-by-zero) 0)) (safe-div 10 0))", "0");

    /* guard — body returns normally */
    test_eval_one("(guard (e ((eq? e 'div-by-zero) 0)) (safe-div 10 2))", "5");

    /* guard — else clause */
    test_eval_one("(guard (e (else 'caught)) (raise 'whatever))", "caught");

    /* guard — multiple clauses, second matches */
    test_eval_one("(guard (e ((eq? e 'a) 1) ((eq? e 'b) 2)) (raise 'b))", "2");

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

void test_compile() {
    compile_init();

    /* Build program as a list of expressions */
    int e3 = cons(read_str("(+ 1 2)"), NIL_VAL);
    int e2 = cons(read_str("(define double (lambda (n) (* n 2)))"), e3);
    int e1 = cons(read_str("(define fact (lambda (n) (if (= n 0) 1 (* n (fact (- n 1))))))"), e2);
    int prog = e1;

    puts_str("--- compiler output ---\n");
    compile_program(prog);
    puts_str("--- end compiler output ---\n");

    puts_str("compile ok\n");
}

void eval_str(char *s) {
    eval(read_str(s), global_env);
}

void load_prelude() {
    /* List operations */
    eval_str("(define map (lambda (f lst) (if (null? lst) lst (cons (f (car lst)) (map f (cdr lst))))))");
    eval_str("(define filter (lambda (p lst) (if (null? lst) lst (if (p (car lst)) (cons (car lst) (filter p (cdr lst))) (filter p (cdr lst))))))");
    eval_str("(define foldr (lambda (f init lst) (if (null? lst) init (f (car lst) (foldr f init (cdr lst))))))");
    eval_str("(define length (lambda (lst) (if (null? lst) 0 (+ 1 (length (cdr lst))))))");
    eval_str("(define append (lambda (a b) (if (null? a) b (cons (car a) (append (cdr a) b)))))");
    eval_str("(define reverse (lambda (lst) (foldr (lambda (x acc) (append acc (list x))) '() lst)))");
    eval_str("(define nth (lambda (n lst) (if (= n 0) (car lst) (nth (- n 1) (cdr lst)))))");

    /* Convenience macros */
    eval_str("(defmacro when (cond body) `(if ,cond ,body nil))");
    eval_str("(defmacro unless (cond body) `(if ,cond nil ,body))");

    /* let: (let ((x 1) (y 2)) body) => ((lambda (x y) body) 1 2) */
    eval_str("(defmacro let (bindings body) `((lambda ,(map car bindings) ,body) ,@(map cadr bindings)))");

    /* and/or (two-arg, short-circuit via if) */
    eval_str("(defmacro and (a b) `(if ,a ,b nil))");
    eval_str("(defmacro or (a b) `(if ,a ,a ,b))");

    /* cond: (cond (test1 expr1) (test2 expr2) ... (t default)) */
    eval_str("(define cond-expand (lambda (clauses) (if (null? clauses) nil (if (eq? (caar clauses) 't) (cadr (car clauses)) `(if ,(caar clauses) ,(cadr (car clauses)) ,(cond-expand (cdr clauses)))))))");
    eval_str("(defmacro cond clauses (cond-expand clauses))");

    /* Threading macros: -> threads x as first arg, ->> as last arg.
     * Two-form version; nest for more: (-> (-> x f) g) */
    eval_str("(defmacro -> (x form) (cons (car form) (cons x (cdr form))))");
    eval_str("(defmacro ->> (x form) (append form (list x)))");

    /* Comparison operators */
    eval_str("(define > (lambda (a b) (< b a)))");
    eval_str("(define >= (lambda (a b) (not (< a b))))");
    eval_str("(define <= (lambda (a b) (not (< b a))))");

    /* Numeric predicates */
    eval_str("(define zero? (lambda (n) (= n 0)))");
    eval_str("(define positive? (lambda (n) (< 0 n)))");
    eval_str("(define negative? (lambda (n) (< n 0)))");
    eval_str("(define abs (lambda (n) (if (< n 0) (- 0 n) n)))");
    eval_str("(define min (lambda (a b) (if (< a b) a b)))");
    eval_str("(define max (lambda (a b) (if (< a b) b a)))");

    /* List accessors */
    eval_str("(define cadr (lambda (x) (car (cdr x))))");
    eval_str("(define caddr (lambda (x) (car (cdr (cdr x)))))");
    eval_str("(define caar (lambda (x) (car (car x))))");
    eval_str("(define cdar (lambda (x) (cdr (car x))))");

    /* Clojure-inspired: reduce (left fold) */
    eval_str("(define reduce (lambda (f init lst) (if (null? lst) init (reduce f (f init (car lst)) (cdr lst)))))");

    /* Function combinators */
    eval_str("(define identity (lambda (x) x))");
    eval_str("(define constantly (lambda (x) (lambda (y) x)))");
    eval_str("(define complement (lambda (f) (lambda (x) (not (f x)))))");
    eval_str("(define compose (lambda (f g) (lambda (x) (f (g x)))))");

    /* Predicate combinators */
    eval_str("(define every? (lambda (p lst) (if (null? lst) t (if (p (car lst)) (every? p (cdr lst)) nil))))");
    eval_str("(define some (lambda (p lst) (if (null? lst) nil (if (p (car lst)) (car lst) (some p (cdr lst))))))");
    eval_str("(define none? (lambda (p lst) (every? (complement p) lst)))");

    /* List construction */
    eval_str("(define range-helper (lambda (i n) (if (= i n) nil (cons i (range-helper (+ i 1) n)))))");
    eval_str("(define range (lambda (n) (range-helper 0 n)))");
    eval_str("(define repeat (lambda (n x) (if (= n 0) nil (cons x (repeat (- n 1) x)))))");
    eval_str("(define take (lambda (n lst) (if (= n 0) nil (if (null? lst) nil (cons (car lst) (take (- n 1) (cdr lst)))))))");
    eval_str("(define drop (lambda (n lst) (if (= n 0) lst (if (null? lst) nil (drop (- n 1) (cdr lst))))))");
    eval_str("(define zip (lambda (a b) (if (null? a) nil (if (null? b) nil (cons (list (car a) (car b)) (zip (cdr a) (cdr b)))))))");
    eval_str("(define flatten (lambda (lst) (if (null? lst) nil (if (pair? (car lst)) (append (flatten (car lst)) (flatten (cdr lst))) (cons (car lst) (flatten (cdr lst)))))))");

    /* Association lists */
    eval_str("(define assoc (lambda (key alist) (if (null? alist) nil (if (eq? key (caar alist)) (car alist) (assoc key (cdr alist))))))");
    eval_str("(define get (lambda (key alist default) (if (null? alist) default (if (eq? key (caar alist)) (cdar alist) (get key (cdr alist) default)))))");

    /* String utilities */
    eval_str("(define ->str (lambda (x) (cond ((string? x) x) ((number? x) (number->string x)) (t \"\"))))");
    eval_str("(define str2 (lambda (a b) (string-append (->str a) (->str b))))");
    eval_str("(define str (lambda args (reduce str2 \"\" args)))");

    /* Lazy sequences: thunk-in-cdr, memoized on force via set! */
    eval_str("(define lazy-cons (lambda (h thk) (cons h (cons 'thunk thk))))");
    eval_str("(define lazy? (lambda (s) (and (pair? s) (and (pair? (cdr s)) (eq? (car (cdr s)) 'thunk)))))");
    eval_str("(define lazy-car car)");
    eval_str("(define lazy-cdr (lambda (s) (if (lazy? s) (let ((v ((cdr (cdr s))))) (begin (set! s (cons (car s) v)) v)) (cdr s))))");
    eval_str("(define lazy-take (lambda (n s) (if (= n 0) nil (if (null? s) nil (cons (lazy-car s) (lazy-take (- n 1) (lazy-cdr s)))))))");
    eval_str("(define lazy-map (lambda (f s) (if (null? s) nil (lazy-cons (f (lazy-car s)) (lambda () (lazy-map f (lazy-cdr s)))))))");
    eval_str("(define lazy-filter (lambda (p s) (if (null? s) nil (if (p (lazy-car s)) (lazy-cons (lazy-car s) (lambda () (lazy-filter p (lazy-cdr s)))) (lazy-filter p (lazy-cdr s))))))");
    eval_str("(define iterate (lambda (f x) (lazy-cons x (lambda () (iterate f (f x))))))");
    eval_str("(define lazy-range (lambda (n) (iterate (lambda (x) (+ x 1)) n)))");
    eval_str("(define take-while (lambda (p lst) (if (null? lst) nil (if (p (car lst)) (cons (car lst) (take-while p (cdr lst))) nil))))");
    eval_str("(define drop-while (lambda (p lst) (if (null? lst) nil (if (p (car lst)) (drop-while p (cdr lst)) lst))))");

    /* Iteration */
    eval_str("(define for-each (lambda (f lst) (if (null? lst) nil (begin (f (car lst)) (for-each f (cdr lst))))))");

    /* Utility functions (Clojure-inspired) */
    eval_str("(define partial (lambda (f . args) (lambda rest (apply f (append args rest)))))");
    eval_str("(define juxt (lambda (f g) (lambda (x) (list (f x) (g x)))))");
    eval_str("(defmacro doseq (binding body) `(for-each (lambda (,(car binding)) ,body) ,(cadr binding)))");
    eval_str("(defmacro dotimes (binding body) `(for-each (lambda (,(car binding)) ,body) (range ,(cadr binding))))");

    /* Trampoline: repeatedly call thunks until non-function result */
    eval_str("(define trampoline (lambda (f) (let ((r (f))) (if (fn? r) (trampoline r) r))))");

    /* Escape continuations */
    eval_str("(define (call/ec proc) (let ((tag (gensym))) (catch tag (proc (lambda (val) (throw tag val))))))");

    /* Error handling: raise / with-handler / guard */
    eval_str("(define *error-tag* (gensym))");
    eval_str("(define *error-handler* nil)");
    eval_str("(define (raise obj) (if (null? *error-handler*) (begin (display \"ERROR: \") (println obj) (exit)) (*error-handler* obj)))");
    eval_str("(define (with-handler handler thunk) (let ((saved *error-handler*)) (catch *error-tag* (begin (set! *error-handler* (lambda (e) (begin (set! *error-handler* saved) (throw *error-tag* (handler e))))) (let ((result (thunk))) (begin (set! *error-handler* saved) result))))))");
    eval_str("(define (error msg) (raise msg))");

    /* guard: (guard (var (test expr) ...) body)
     * Expands to with-handler that pattern-matches the error value */
    eval_str("(define (guard-clauses var clauses) (if (null? clauses) '(raise e) (let ((clause (car clauses))) (if (eq? (car clause) 'else) (cadr clause) `(if ,(car clause) ,(cadr clause) ,(guard-clauses var (cdr clauses)))))))");
    eval_str("(defmacro guard (binding body) `(with-handler (lambda (,(car binding)) ,(guard-clauses (car binding) (cdr binding))) (lambda () ,body)))");

    /* unwind-protect: (unwind-protect body cleanup...)
     * Evaluates body, then cleanup forms. Cleanup runs even on non-local exit. */
    eval_str("(defmacro unwind-protect (body cleanup) `(dynamic-wind (lambda () nil) (lambda () ,body) (lambda () ,cleanup)))");

    /* COR24-TB I/O addresses */
    eval_str("(define IO-LED #xFF0000)");
    eval_str("(define IO-SWITCH #xFF0000)");
    eval_str("(define IO-UART-DATA #xFF0100)");
    eval_str("(define IO-UART-STATUS #xFF0101)");
    eval_str("(define IO-INT-ENABLE #xFF0010)");

    /* I/O convenience functions */
    eval_str("(define set-leds (lambda (n) (poke IO-LED n)))");
    eval_str("(define get-leds (lambda () (peek IO-LED)))");
    eval_str("(define s2-pressed? (lambda () (= (% (peek IO-SWITCH) 2) 0)))");
}

void repl() {
    char line[1024];
    puts_str("> ");
    while (1) {
        int len = read_line(line, 1024);
        if (len < 0) {
            puts_str("Bye.\n");
            halt();
        }
        if (len == 0) {
            puts_str("> ");
            continue;
        }
        int expr = read_str(line);
        int result = eval(expr, global_env);
        print_val(result);
        putc_uart('\n');
        puts_str("> ");
    }
}

int main() {
    heap_init();
    gc_init();
    symbol_init();
    string_init();
    eval_init();
    gc_enabled = 1;  /* enable GC after init */
    load_prelude();
    test_scaffold();
    test_reader();
    test_eval();
    test_gc();
    test_compile();
    repl();
    return 0;
}
