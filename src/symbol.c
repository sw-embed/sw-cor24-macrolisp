#include "tml.h"
#include <string.h>

/*
 * Symbol storage: a simple array of name strings indexed by symbol id.
 * Symbol 0 = "nil", symbol 1 = "t".
 * intern() does linear search — fine for a small system.
 */

#define MAX_SYMBOLS 512
#define MAX_NAME_CHARS 4096

static char name_pool[MAX_NAME_CHARS];
static int name_pool_next;

static const char *sym_names[MAX_SYMBOLS];
static int sym_count;

void symbol_init(void) {
    name_pool_next = 0;
    sym_count = 0;

    /* Pre-intern nil (index 0) and t (index 1) */
    intern("nil");
    intern("t");
}

static const char *store_name(const char *name) {
    int len = (int)strlen(name);
    if (name_pool_next + len + 1 > MAX_NAME_CHARS) {
        platform_putchar('S');
        platform_putchar('Y');
        platform_putchar('M');
        platform_putchar('\n');
        platform_halt(1);
    }
    char *dst = &name_pool[name_pool_next];
    memcpy(dst, name, (size_t)(len + 1));
    name_pool_next += len + 1;
    return dst;
}

Value intern(const char *name) {
    /* Search existing symbols */
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(sym_names[i], name) == 0) {
            return MAKE_SYMBOL(i);
        }
    }

    /* Create new symbol */
    if (sym_count >= MAX_SYMBOLS) {
        platform_putchar('S');
        platform_putchar('Y');
        platform_putchar('M');
        platform_putchar('\n');
        platform_halt(1);
    }

    int idx = sym_count++;
    sym_names[idx] = store_name(name);
    return MAKE_SYMBOL(idx);
}

const char *symbol_name(Value sym) {
    int idx = (int)PTR_IDX(sym);
    if (idx < 0 || idx >= sym_count) {
        return "???";
    }
    return sym_names[idx];
}
