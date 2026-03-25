#pragma once

/* prelude-full.h -- Everything: standard + lazy, threading, anaphora, etc.
 * Recommend --stack-kilobytes 8 for deep call chains. */

void load_prelude() {
    /* === Standard prelude (inlined, not #included, for single-compilation) === */

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
    eval_str("(defmacro when (cond . body) `(if ,cond (begin ,@body) nil))");
    eval_str("(defmacro unless (cond . body) `(if ,cond nil (begin ,@body)))");
    eval_str("(define (let-expand first rest) (if (pair? first) `((lambda ,(map car first) ,@rest) ,@(map cadr first)) `((lambda (,first) (set! ,first (lambda ,(map car (car rest)) ,@(cdr rest))) (,first ,@(map cadr (car rest)))) nil)))");
    eval_str("(defmacro let (first . rest) (let-expand first rest))");
    eval_str("(defmacro and (a b) `(if ,a ,b nil))");
    eval_str("(defmacro or (a b) `(if ,a ,a ,b))");
    eval_str("(define cond-expand (lambda (clauses) (if (null? clauses) nil (if (eq? (caar clauses) 't) (cadr (car clauses)) `(if ,(caar clauses) ,(cadr (car clauses)) ,(cond-expand (cdr clauses)))))))");
    eval_str("(defmacro cond clauses (cond-expand clauses))");

    /* Threading macros */
    eval_str("(defmacro -> (x form) (cons (car form) (cons x (cdr form))))");
    eval_str("(defmacro ->> (x form) (append form (list x)))");

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

    /* String utilities */
    eval_str("(define ->str (lambda (x) (cond ((string? x) x) ((number? x) (number->string x)) (t \"\"))))");
    eval_str("(define str2 (lambda (a b) (string-append (->str a) (->str b))))");
    eval_str("(define str (lambda args (reduce str2 \"\" args)))");

    /* Iteration */
    eval_str("(define for-each (lambda (f lst) (if (null? lst) nil (begin (f (car lst)) (for-each f (cdr lst))))))");

    /* Utility functions */
    eval_str("(define partial (lambda (f . args) (lambda rest (apply f (append args rest)))))");
    eval_str("(define juxt (lambda (f g) (lambda (x) (list (f x) (g x)))))");
    eval_str("(defmacro doseq (binding body) `(for-each (lambda (,(car binding)) ,body) ,(cadr binding)))");
    eval_str("(defmacro dotimes (binding body) `(for-each (lambda (,(car binding)) ,body) (range ,(cadr binding))))");

    /* Association lists */
    eval_str("(define assoc (lambda (key alist) (if (null? alist) nil (if (eq? key (caar alist)) (car alist) (assoc key (cdr alist))))))");
    eval_str("(define get (lambda (key alist default) (if (null? alist) default (if (eq? key (caar alist)) (cdar alist) (get key (cdr alist) default)))))");

    /* Escape continuations */
    eval_str("(define (call/ec proc) (let ((tag (gensym))) (catch tag (proc (lambda (val) (throw tag val))))))");

    /* Error handling */
    eval_str("(define *error-tag* (gensym))");
    eval_str("(define *error-handler* nil)");
    eval_str("(define (raise obj) (if (null? *error-handler*) (begin (display \"ERROR: \") (println obj) (exit)) (*error-handler* obj)))");
    eval_str("(define (with-handler handler thunk) (let ((saved *error-handler*)) (begin (set! *error-handler* (lambda (e) (throw *error-tag* (handler e)))) (let ((result (catch *error-tag* (thunk)))) (begin (set! *error-handler* saved) result)))))");
    eval_str("(define (error msg) (raise msg))");
    eval_str("(define (guard-clauses var clauses) (if (null? clauses) '(raise e) (let ((clause (car clauses))) (if (eq? (car clause) 'else) (cadr clause) `(if ,(car clause) ,(cadr clause) ,(guard-clauses var (cdr clauses)))))))");
    eval_str("(defmacro guard (binding body) `(with-handler (lambda (,(car binding)) ,(guard-clauses (car binding) (cdr binding))) (lambda () ,body)))");
    eval_str("(defmacro unwind-protect (body cleanup) `(dynamic-wind (lambda () nil) (lambda () ,body) (lambda () ,cleanup)))");

    /* Restartable conditions */
    eval_str("(define *restarts* nil)");
    eval_str("(define (with-restart name handler thunk) (let ((tag (gensym))) (let ((saved *restarts*)) (begin (set! *restarts* (cons (list name tag handler) *restarts*)) (let ((result (catch tag (let ((v (thunk))) (begin (set! *restarts* saved) v))))) (begin (set! *restarts* saved) result))))))");
    eval_str("(define (invoke-restart name val) (let ((r (assoc name *restarts*))) (if (null? r) (begin (display \"ERR:no-restart \") (println name)) (let ((tag (cadr r))) (let ((handler (caddr r))) (throw tag (handler val)))))))");

    /* Dynamic parameters */
    eval_str("(define (make-parameter init) (let ((val init)) (lambda args (if (null? args) val (set! val (car args))))))");
    eval_str("(define (call-with-parameterize param new-val thunk) (let ((saved (param))) (dynamic-wind (lambda () (param new-val)) thunk (lambda () (param saved)))))");
    eval_str("(defmacro parameterize (bindings body) `(call-with-parameterize ,(caar bindings) ,(cadr (car bindings)) (lambda () ,body)))");

    /* Trampoline */
    eval_str("(define trampoline (lambda (f) (let ((r (f))) (if (fn? r) (trampoline r) r))))");

    /* Lazy sequences */
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

    /* Anaphoric macros */
    eval_str("(defmacro aif (test then else) `(let ((it ,test)) (if it ,then ,else)))");
    eval_str("(defmacro awhen (test body) `(let ((it ,test)) (if it ,body nil)))");
    eval_str("(defmacro aand (a b) `(let ((it ,a)) (if it ,b nil)))");

    /* Boolean aliases */
    eval_str("(define true t)");
    eval_str("(define false nil)");

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
