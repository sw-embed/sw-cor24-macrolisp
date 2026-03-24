#pragma once

/* gc.h -- Mark-sweep garbage collector
 *
 * Cells are allocated from a free list. When the free list is empty,
 * GC marks all reachable cells from roots, then sweeps unmarked cells
 * back onto the free list.
 *
 * Roots: global_env, pre-interned symbols, and the explicit root stack
 * (for protecting temporaries across allocations).
 */

#define MAX_GC_ROOTS 64

int heap_mark[HEAP_SIZE];
int free_list;      /* head of free list (cell index), -1 = empty */
int gc_roots[MAX_GC_ROOTS];
int gc_root_count;
int gc_collections;  /* statistics */

void gc_init() {
    free_list = -1;
    gc_root_count = 0;
    gc_collections = 0;
}

/* --- Root stack for protecting temporaries --- */

int gc_protect(int val) {
    if (gc_root_count >= MAX_GC_ROOTS) {
        puts_str("GC:roots\n");
        asm("_gcr_halt:");
        asm("bra _gcr_halt");
    }
    gc_roots[gc_root_count] = val;
    gc_root_count = gc_root_count + 1;
    return val;
}

void gc_unprotect(int n) {
    gc_root_count = gc_root_count - n;
}

/* --- Mark phase --- */

void gc_mark_val(int v) {
    /* Only cons and extended values reference heap cells */
    if (IS_CONS(v)) {
        int idx = PTR_IDX(v);
        if (heap_mark[idx]) return;  /* already marked */
        heap_mark[idx] = 1;
        gc_mark_val(heap_car[idx]);
        gc_mark_val(heap_cdr[idx]);
    } else if (IS_EXTENDED(v)) {
        int idx = PTR_IDX(v);
        if (heap_mark[idx]) return;
        heap_mark[idx] = 1;
        /* Extended objects store type in car, data in cdr.
         * Data may reference other heap cells (closures, macros). */
        gc_mark_val(heap_car[idx]);
        gc_mark_val(heap_cdr[idx]);
    }
}

/* --- Sweep phase --- */

void gc_sweep() {
    free_list = -1;
    int i = heap_next - 1;
    while (i >= 0) {
        if (heap_mark[i]) {
            heap_mark[i] = 0;  /* clear for next cycle */
        } else {
            /* Add to free list */
            heap_cdr[i] = free_list;
            heap_car[i] = NIL_VAL;  /* clear for safety */
            free_list = i;
        }
        i = i - 1;
    }
}

/* --- Collect --- */

void gc_collect() {
    gc_collections = gc_collections + 1;

    /* Clear marks */
    int i = 0;
    while (i < heap_next) {
        heap_mark[i] = 0;
        i = i + 1;
    }

    /* Mark from global environment */
    gc_mark_val(global_env);

    /* Mark pre-interned special form symbols' values
     * (symbols themselves are not heap-allocated, but their
     *  bindings in global_env are already covered above) */

    /* Mark root stack */
    i = 0;
    while (i < gc_root_count) {
        gc_mark_val(gc_roots[i]);
        i = i + 1;
    }

    /* Sweep */
    gc_sweep();
}

/* --- Allocation with GC --- */

int gc_alloc_cell() {
    /* Try free list first */
    if (free_list >= 0) {
        int idx = free_list;
        free_list = heap_cdr[idx];
        heap_car[idx] = NIL_VAL;
        heap_cdr[idx] = NIL_VAL;
        return idx;
    }

    /* Try bump allocator */
    if (heap_next < HEAP_SIZE) {
        int idx = heap_next;
        heap_next = heap_next + 1;
        heap_car[idx] = NIL_VAL;
        heap_cdr[idx] = NIL_VAL;
        return idx;
    }

    /* Out of space: collect and retry free list */
    gc_collect();

    if (free_list >= 0) {
        int idx = free_list;
        free_list = heap_cdr[idx];
        heap_car[idx] = NIL_VAL;
        heap_cdr[idx] = NIL_VAL;
        return idx;
    }

    /* Truly out of memory */
    puts_str("OOM\n");
    asm("_oom2_halt:");
    asm("bra _oom2_halt");
    return 0;
}
