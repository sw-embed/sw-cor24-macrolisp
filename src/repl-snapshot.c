/* tml24c REPL — snapshot-accelerated standard prelude
 *
 * If a snapshot is loaded at SNAPSHOT_ADDR via --load-binary,
 * restores interpreter state from it (skipping prelude eval).
 * Otherwise falls back to normal prelude loading.
 *
 * Build: just build-snapshot
 * Fast:  just run-fast (uses pre-built snapshot)
 * Slow:  cor24-run --run build/repl-snapshot.s --terminal --echo --speed 0
 *        (no --load-binary → falls back to eval_str prelude)
 */

#include "tml.h"
#include "io.h"
#include "heap.h"
#include "symbol.h"
#include "string.h"
#include "print.h"
#include "read.h"
#include "eval.h"
#include "gc.h"
#include "snapshot.h"

void eval_str(char *s) { eval(read_str(s), global_env); }

#include "prelude-standard.h"

void repl() {
    char line[1024];
    puts_str("> ");
    while (1) {
        int len = read_line(line, 1024);
        if (len < 0) { puts_str("Bye.\n"); halt(); }
        if (len == 0) { puts_str("> "); continue; }
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

    /* Try to restore from snapshot at SNAPSHOT_ADDR */
    if (snapshot_restore(SNAPSHOT_ADDR)) {
        /* Snapshot restored — skip prelude eval */
    } else {
        /* No snapshot — fall back to normal prelude loading */
        gc_enabled = 1;
        load_prelude();
    }

    repl();
    return 0;
}
