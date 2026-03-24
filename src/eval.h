#pragma once

/* eval.h -- Evaluator with core special forms
 *
 * Special forms: quote, if, define, lambda, defmacro, begin
 * Environment: alist of (symbol . value) pairs
 * Closures/macros: extended heap objects
 */

/* Forward declarations */
int eval(int expr, int env);
int apply_fn(int fn, int args);

/* Global environment */
int global_env;

/* Pre-interned special form symbols */
int sym_quote;
int sym_if;
int sym_define;
int sym_lambda;
int sym_defmacro;
int sym_begin;
/* Primitive IDs */
#define PRIM_ADD     0
#define PRIM_SUB     1
#define PRIM_MUL     2
#define PRIM_DIV     3
#define PRIM_MOD     4
#define PRIM_LT      5
#define PRIM_EQ_NUM  6
#define PRIM_CONS    7
#define PRIM_CAR     8
#define PRIM_CDR     9
#define PRIM_LIST   10
#define PRIM_NULLP  11
#define PRIM_PAIRP  12
#define PRIM_ATOMP  13
#define PRIM_EQP    14
#define PRIM_NOT    15
#define PRIM_PRINT  16
#define PRIM_NUMP   17
#define PRIM_EXIT   18
#define PRIM_PEEK   19
#define PRIM_POKE   20
#define PRIM_DELAY  21
#define PRIM_PRINTLN 22
#define PRIM_NEWLINE 23

/* --- Extended object accessors --- */

int ext_type(int v) {
    return heap_car[PTR_IDX(v)];
}

int ext_data(int v) {
    return heap_cdr[PTR_IDX(v)];
}

/* --- Extended object constructors --- */

int make_closure(int params, int body, int env) {
    int inner = cons(body, env);
    int data = cons(params, inner);
    int idx = alloc_cell();
    heap_car[idx] = ETYPE_CLOSURE;
    heap_cdr[idx] = data;
    return MAKE_EXTENDED(idx);
}

int make_macro(int params, int body, int env) {
    int inner = cons(body, env);
    int data = cons(params, inner);
    int idx = alloc_cell();
    heap_car[idx] = ETYPE_MACRO;
    heap_cdr[idx] = data;
    return MAKE_EXTENDED(idx);
}

int make_primitive(int id) {
    int idx = alloc_cell();
    heap_car[idx] = ETYPE_PRIMITIVE;
    heap_cdr[idx] = MAKE_FIXNUM(id);
    return MAKE_EXTENDED(idx);
}

/* Closure/macro field access */
int closure_params(int v) { return car(ext_data(v)); }
int closure_body(int v)   { return car(cdr(ext_data(v))); }
int closure_env(int v)    { return cdr(cdr(ext_data(v))); }

/* --- Environment --- */

int env_extend(int sym, int val, int env) {
    return cons(cons(sym, val), env);
}

int env_lookup(int sym, int env) {
    int e = env;
    while (!IS_NIL(e)) {
        int binding = car(e);
        if (car(binding) == sym) {
            return cdr(binding);
        }
        e = cdr(e);
    }
    e = global_env;
    while (!IS_NIL(e)) {
        int binding = car(e);
        if (car(binding) == sym) {
            return cdr(binding);
        }
        e = cdr(e);
    }
    puts_str("ERR:unbound ");
    print_val(sym);
    putc_uart(10);
    return NIL_VAL;
}

int env_bind(int params, int args, int env) {
    while (!IS_NIL(params)) {
        env = env_extend(car(params), car(args), env);
        params = cdr(params);
        args = cdr(args);
    }
    return env;
}

/* --- Eval --- */

int eval_list(int list, int env) {
    if (IS_NIL(list)) return NIL_VAL;
    int val = eval(car(list), env);
    int rest = eval_list(cdr(list), env);
    return cons(val, rest);
}

int apply_primitive(int id, int args) {
    int a = NIL_VAL;
    int b = NIL_VAL;

    if (!IS_NIL(args)) {
        a = car(args);
        if (!IS_NIL(cdr(args))) {
            b = car(cdr(args));
        }
    }

    if (id == PRIM_ADD) return MAKE_FIXNUM(FIXNUM_VAL(a) + FIXNUM_VAL(b));
    if (id == PRIM_SUB) return MAKE_FIXNUM(FIXNUM_VAL(a) - FIXNUM_VAL(b));
    if (id == PRIM_MUL) return MAKE_FIXNUM(FIXNUM_VAL(a) * FIXNUM_VAL(b));
    if (id == PRIM_DIV) return MAKE_FIXNUM(FIXNUM_VAL(a) / FIXNUM_VAL(b));
    if (id == PRIM_MOD) return MAKE_FIXNUM(FIXNUM_VAL(a) % FIXNUM_VAL(b));
    if (id == PRIM_LT) {
        if (FIXNUM_VAL(a) < FIXNUM_VAL(b)) return T_VAL;
        return NIL_VAL;
    }
    if (id == PRIM_EQ_NUM) {
        if (FIXNUM_VAL(a) == FIXNUM_VAL(b)) return T_VAL;
        return NIL_VAL;
    }
    if (id == PRIM_CONS) return cons(a, b);
    if (id == PRIM_CAR) return car(a);
    if (id == PRIM_CDR) return cdr(a);
    if (id == PRIM_LIST) return args;
    if (id == PRIM_NULLP) {
        if (IS_NIL(a)) return T_VAL;
        return NIL_VAL;
    }
    if (id == PRIM_PAIRP) {
        if (IS_CONS(a)) return T_VAL;
        return NIL_VAL;
    }
    if (id == PRIM_ATOMP) {
        if (!IS_CONS(a)) return T_VAL;
        return NIL_VAL;
    }
    if (id == PRIM_EQP) {
        if (a == b) return T_VAL;
        return NIL_VAL;
    }
    if (id == PRIM_NOT) {
        if (IS_NIL(a)) return T_VAL;
        return NIL_VAL;
    }
    if (id == PRIM_PRINT) {
        print_val(a);
        return a;
    }
    if (id == PRIM_PRINTLN) {
        print_val(a);
        putc_uart('\n');
        return a;
    }
    if (id == PRIM_NEWLINE) {
        putc_uart('\n');
        return NIL_VAL;
    }
    if (id == PRIM_NUMP) {
        if (IS_FIXNUM(a)) return T_VAL;
        return NIL_VAL;
    }
    if (id == PRIM_EXIT) {
        puts_str("Bye.\n");
        halt();
    }
    if (id == PRIM_PEEK) {
        return MAKE_FIXNUM(*(char *)FIXNUM_VAL(a));
    }
    if (id == PRIM_POKE) {
        *(char *)FIXNUM_VAL(a) = FIXNUM_VAL(b);
        return b;
    }
    if (id == PRIM_DELAY) {
        /* Spin-loop delay calibrated for --speed 500000 (500K IPS).
         * The loop body compiles to ~10 instructions on COR24.
         * 500 instructions/ms ÷ 10 instructions/iter = 50 iters/ms. */
        int ms = FIXNUM_VAL(a);
        int iters = ms * 50;
        while (iters > 0) {
            iters = iters - 1;
        }
        return NIL_VAL;
    }

    return NIL_VAL;
}

int eval(int expr, int env) {
    /* Self-evaluating */
    if (IS_FIXNUM(expr)) return expr;
    if (IS_NIL(expr)) return expr;
    if (expr == T_VAL) return expr;
    if (IS_EXTENDED(expr)) return expr;

    /* Symbol lookup */
    if (IS_SYMBOL(expr)) {
        return env_lookup(expr, env);
    }

    /* List: special form or function call */
    if (!IS_CONS(expr)) return expr;

    int head = car(expr);
    int args = cdr(expr);

    /* quote */
    if (head == sym_quote) {
        return car(args);
    }

    /* if */
    if (head == sym_if) {
        int cond_val = eval(car(args), env);
        if (!IS_NIL(cond_val)) {
            return eval(car(cdr(args)), env);
        }
        int else_part = cdr(cdr(args));
        if (IS_NIL(else_part)) return NIL_VAL;
        return eval(car(else_part), env);
    }

    /* define */
    if (head == sym_define) {
        int sym = car(args);
        int val = eval(car(cdr(args)), env);
        global_env = env_extend(sym, val, global_env);
        return val;
    }

    /* lambda */
    if (head == sym_lambda) {
        int params = car(args);
        int body = car(cdr(args));
        return make_closure(params, body, env);
    }

    /* defmacro: (defmacro name (params) body) */
    if (head == sym_defmacro) {
        int name = car(args);
        int mparams = car(cdr(args));
        int mbody = car(cdr(cdr(args)));
        int mac = make_macro(mparams, mbody, env);
        global_env = env_extend(name, mac, global_env);
        return mac;
    }

    /* begin */
    if (head == sym_begin) {
        int result = NIL_VAL;
        while (!IS_NIL(args)) {
            result = eval(car(args), env);
            args = cdr(args);
        }
        return result;
    }

    /* Function call: eval head */
    int fn = eval(head, env);

    /* Macro expansion: apply with unevaluated args, then eval result */
    if (IS_EXTENDED(fn)) {
        if (ext_type(fn) == ETYPE_MACRO) {
            int mac_env = env_bind(closure_params(fn), args, closure_env(fn));
            int expanded = eval(closure_body(fn), mac_env);
            return eval(expanded, env);
        }
    }

    /* Evaluate arguments */
    int evaled_args = eval_list(args, env);

    return apply_fn(fn, evaled_args);
}

int apply_fn(int fn, int args) {
    if (IS_EXTENDED(fn)) {
        int fn_type = ext_type(fn);
        if (fn_type == ETYPE_CLOSURE) {
            int bound_env = env_bind(closure_params(fn), args, closure_env(fn));
            return eval(closure_body(fn), bound_env);
        }
        if (fn_type == ETYPE_PRIMITIVE) {
            int id = FIXNUM_VAL(ext_data(fn));
            return apply_primitive(id, args);
        }
    }
    puts_str("ERR:not-fn\n");
    return NIL_VAL;
}

/* --- Initialization --- */

void register_prim(char *name, int id) {
    int sym = intern(name);
    int prim = make_primitive(id);
    global_env = env_extend(sym, prim, global_env);
}

void eval_init() {
    global_env = NIL_VAL;

    sym_quote = intern("quote");
    sym_if = intern("if");
    sym_define = intern("define");
    sym_lambda = intern("lambda");
    sym_defmacro = intern("defmacro");
    sym_begin = intern("begin");

    register_prim("+", PRIM_ADD);
    register_prim("-", PRIM_SUB);
    register_prim("*", PRIM_MUL);
    register_prim("/", PRIM_DIV);
    register_prim("%", PRIM_MOD);
    register_prim("<", PRIM_LT);
    register_prim("=", PRIM_EQ_NUM);
    register_prim("cons", PRIM_CONS);
    register_prim("car", PRIM_CAR);
    register_prim("cdr", PRIM_CDR);
    register_prim("list", PRIM_LIST);
    register_prim("null?", PRIM_NULLP);
    register_prim("pair?", PRIM_PAIRP);
    register_prim("atom?", PRIM_ATOMP);
    register_prim("eq?", PRIM_EQP);
    register_prim("not", PRIM_NOT);
    register_prim("print", PRIM_PRINT);
    register_prim("number?", PRIM_NUMP);
    register_prim("exit", PRIM_EXIT);
    register_prim("peek", PRIM_PEEK);
    register_prim("poke", PRIM_POKE);
    register_prim("delay", PRIM_DELAY);
    register_prim("println", PRIM_PRINTLN);
    register_prim("newline", PRIM_NEWLINE);
}
