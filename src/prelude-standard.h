#pragma once

/* prelude-standard.h -- Core Lisp functions.
 * Works with --stack-kilobytes 3 (default). */

void load_prelude() {
    /* List operations */
    eval_str("(define map (lambda (f lst) (if (null? lst) lst (cons (f (car lst)) (map f (cdr lst))))))");
    eval_str("(define filter (lambda (p lst) (if (null? lst) lst (if (p (car lst)) (cons (car lst) (filter p (cdr lst))) (filter p (cdr lst))))))");
    eval_str("(define reduce (lambda (f init lst) (if (null? lst) init (reduce f (f init (car lst)) (cdr lst)))))");
    eval_str("(define foldr (lambda (f init lst) (if (null? lst) init (f (car lst) (foldr f init (cdr lst))))))");
    eval_str("(define length (lambda (lst) (if (null? lst) 0 (+ 1 (length (cdr lst))))))");
    eval_str("(define append (lambda (a b) (if (null? a) b (cons (car a) (append (cdr a) b)))))");
    eval_str("(define reverse (lambda (lst) (reduce (lambda (acc x) (cons x acc)) nil lst)))");
    eval_str("(define nth (lambda (n lst) (if (= n 0) (car lst) (nth (- n 1) (cdr lst)))))");

    /* Macros */
    eval_str("(defmacro when (cond body) `(if ,cond ,body nil))");
    eval_str("(defmacro unless (cond body) `(if ,cond nil ,body))");
    eval_str("(defmacro let (bindings body) `((lambda ,(map car bindings) ,body) ,@(map cadr bindings)))");
    eval_str("(defmacro and (a b) `(if ,a ,b nil))");
    eval_str("(defmacro or (a b) `(if ,a ,a ,b))");
    eval_str("(define cond-expand (lambda (clauses) (if (null? clauses) nil (if (eq? (caar clauses) 't) (cadr (car clauses)) `(if ,(caar clauses) ,(cadr (car clauses)) ,(cond-expand (cdr clauses)))))))");
    eval_str("(defmacro cond clauses (cond-expand clauses))");

    /* Comparison */
    eval_str("(define > (lambda (a b) (< b a)))");
    eval_str("(define >= (lambda (a b) (not (< a b))))");
    eval_str("(define <= (lambda (a b) (not (< b a))))");

    /* Numeric */
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

    /* Function combinators */
    eval_str("(define identity (lambda (x) x))");
    eval_str("(define complement (lambda (f) (lambda (x) (not (f x)))))");
    eval_str("(define compose (lambda (f g) (lambda (x) (f (g x)))))");

    /* String utilities */
    eval_str("(define ->str (lambda (x) (cond ((string? x) x) ((number? x) (number->string x)) (t \"\"))))");
    eval_str("(define str2 (lambda (a b) (string-append (->str a) (->str b))))");
    eval_str("(define str (lambda args (reduce str2 \"\" args)))");

    /* Iteration */
    eval_str("(define for-each (lambda (f lst) (if (null? lst) nil (begin (f (car lst)) (for-each f (cdr lst))))))");

    /* Association lists */
    eval_str("(define assoc (lambda (key alist) (if (null? alist) nil (if (eq? key (caar alist)) (car alist) (assoc key (cdr alist))))))");
    eval_str("(define get (lambda (key alist default) (if (null? alist) default (if (eq? key (caar alist)) (cdar alist) (get key (cdr alist) default)))))");

    /* Escape continuations */
    eval_str("(define (call/ec proc) (let ((tag (gensym))) (catch tag (proc (lambda (val) (throw tag val))))))");

    /* Error handling */
    eval_str("(define *error-tag* (gensym))");
    eval_str("(define *error-handler* nil)");
    eval_str("(define (raise obj) (if (null? *error-handler*) (begin (display \"ERROR: \") (println obj) (exit)) (*error-handler* obj)))");
    eval_str("(define (with-handler handler thunk) (let ((saved *error-handler*)) (catch *error-tag* (begin (set! *error-handler* (lambda (e) (begin (set! *error-handler* saved) (throw *error-tag* (handler e))))) (let ((result (thunk))) (begin (set! *error-handler* saved) result))))))");
    eval_str("(define (error msg) (raise msg))");
    eval_str("(define (guard-clauses var clauses) (if (null? clauses) '(raise e) (let ((clause (car clauses))) (if (eq? (car clause) 'else) (cadr clause) `(if ,(car clause) ,(cadr clause) ,(guard-clauses var (cdr clauses)))))))");
    eval_str("(defmacro guard (binding body) `(with-handler (lambda (,(car binding)) ,(guard-clauses (car binding) (cdr binding))) (lambda () ,body)))");
    eval_str("(defmacro unwind-protect (body cleanup) `(dynamic-wind (lambda () nil) (lambda () ,body) (lambda () ,cleanup)))");

    /* Comments */
    eval_str("(defmacro comment rest nil)");

    /* Metaprogramming */
    eval_str("(define macroexpand (lambda (form) (let ((expanded (macroexpand-1 form))) (if (eq? expanded form) form (macroexpand expanded)))))");

    /* I/O constants */
    eval_str("(define IO-LED #xFF0000)");
    eval_str("(define IO-SWITCH #xFF0000)");
    eval_str("(define IO-UART-DATA #xFF0100)");
    eval_str("(define IO-UART-STATUS #xFF0101)");
    eval_str("(define IO-INT-ENABLE #xFF0010)");
    eval_str("(define set-leds (lambda (n) (poke IO-LED n)))");
    eval_str("(define get-leds (lambda () (peek IO-LED)))");
    eval_str("(define s2-pressed? (lambda () (= (% (peek IO-SWITCH) 2) 0)))");
}
