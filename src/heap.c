#include "tml.h"

static Cell heap[HEAP_SIZE];
static int heap_next;  /* bump allocator pointer */

void heap_init(void) {
    heap_next = 2;  /* reserve index 0 for nil, 1 for t */
}

Value alloc_cell(void) {
    if (heap_next >= HEAP_SIZE) {
        /* TODO: trigger GC, then retry */
        platform_putchar('O');
        platform_putchar('O');
        platform_putchar('M');
        platform_putchar('\n');
        platform_halt(1);
    }
    int idx = heap_next++;
    heap[idx].car = NIL_VAL;
    heap[idx].cdr = NIL_VAL;
    return idx;  /* raw index — caller wraps with MAKE_CONS etc. */
}

Value alloc_cells(int n) {
    if (heap_next + n > HEAP_SIZE) {
        platform_putchar('O');
        platform_putchar('O');
        platform_putchar('M');
        platform_putchar('\n');
        platform_halt(1);
    }
    int idx = heap_next;
    for (int i = 0; i < n; i++) {
        heap[idx + i].car = NIL_VAL;
        heap[idx + i].cdr = NIL_VAL;
    }
    heap_next += n;
    return idx;
}

Cell *cell_at(Value v) {
    return &heap[PTR_IDX(v)];
}

Value cons(Value car, Value cdr) {
    int idx = (int)alloc_cell();
    heap[idx].car = car;
    heap[idx].cdr = cdr;
    return MAKE_CONS(idx);
}
