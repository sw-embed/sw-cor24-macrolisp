/* tml24c REPL mode -- no tests, straight to interactive eval */

#include "tml.h"
#include "io.h"
#include "heap.h"
#include "symbol.h"
#include "string.h"
#include "print.h"
#include "read.h"
#include "eval.h"
#include "gc.h"

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
            /* Ctrl-D: EOF */
            puts_str("Bye.\n");
            halt();
        }
        if (len == 0) {
            puts_str("> ");
            continue;
        }
        int expr = read_str(line);
        gc_protect(expr);
        int result = eval(expr, global_env);
        gc_unprotect(1);
        gc_protect(result);
        print_val(result);
        gc_unprotect(1);
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
    gc_enabled = 1;
    load_prelude();
    repl();
    return 0;
}
