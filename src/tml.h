#ifndef TML_H
#define TML_H

#include <stdint.h>

/* ---------- Value type ---------- */

/*
 * Every Lisp value is a 32-bit word on host (24-bit on COR24).
 * Low 2 bits are the tag; upper bits are the payload.
 *
 *   [31..2] payload  [1:0] tag
 *   00  Fixnum   — 30-bit signed integer (22-bit on COR24)
 *   01  Cons     — heap cell index
 *   10  Symbol   — heap cell index
 *   11  Extended — heap cell index (closure, macro, primitive, string)
 */
typedef int32_t Value;

/* Tags */
#define TAG_FIXNUM   0
#define TAG_CONS     1
#define TAG_SYMBOL   2
#define TAG_EXTENDED 3
#define TAG_MASK     3

/* Tag checks */
#define IS_FIXNUM(v)   (((v) & TAG_MASK) == TAG_FIXNUM)
#define IS_CONS(v)     (((v) & TAG_MASK) == TAG_CONS)
#define IS_SYMBOL(v)   (((v) & TAG_MASK) == TAG_SYMBOL)
#define IS_EXTENDED(v) (((v) & TAG_MASK) == TAG_EXTENDED)

/* Fixnum encode/decode */
#define MAKE_FIXNUM(n)  ((Value)((n) << 2) | TAG_FIXNUM)
#define FIXNUM_VAL(v)   ((v) >> 2)

/* Pointer (cell index) encode/decode */
#define MAKE_CONS(idx)     ((Value)((idx) << 2) | TAG_CONS)
#define MAKE_SYMBOL(idx)   ((Value)((idx) << 2) | TAG_SYMBOL)
#define MAKE_EXTENDED(idx) ((Value)((idx) << 2) | TAG_EXTENDED)
#define PTR_IDX(v)         ((v) >> 2)

/* ---------- Special values ---------- */

/* nil is symbol index 0, t is symbol index 1 */
#define NIL_VAL   MAKE_SYMBOL(0)
#define T_VAL     MAKE_SYMBOL(1)
#define IS_NIL(v) ((v) == NIL_VAL)

/* ---------- Heap ---------- */

typedef struct {
    Value car;
    Value cdr;
} Cell;

#define HEAP_SIZE 8192

/* Extended object type tags (stored in cdr of second cell) */
#define ETYPE_CLOSURE   MAKE_FIXNUM(1)
#define ETYPE_MACRO     MAKE_FIXNUM(2)
#define ETYPE_PRIMITIVE MAKE_FIXNUM(3)
#define ETYPE_STRING    MAKE_FIXNUM(4)

/* Heap API */
void heap_init(void);
Value alloc_cell(void);
Value alloc_cells(int n);
Cell *cell_at(Value v);  /* get Cell* from any pointer-tagged value */

/* Accessors for cons cells */
#define CAR(v)  (cell_at(v)->car)
#define CDR(v)  (cell_at(v)->cdr)

/* Cons constructor */
Value cons(Value car, Value cdr);

/* ---------- Symbols ---------- */

void symbol_init(void);
Value intern(const char *name);
const char *symbol_name(Value sym);

/* ---------- Environment ---------- */

Value env_create(void);
Value env_lookup(Value sym, Value env);
void  env_define(Value sym, Value val, Value env);
void  env_set(Value sym, Value val, Value env);
Value env_extend(Value params, Value args, Value env);

/* ---------- Platform ---------- */

void platform_putchar(int ch);
int  platform_getchar(void);
void platform_halt(int code);

/* ---------- Printer ---------- */

void print_val(Value v);

#endif /* TML_H */
