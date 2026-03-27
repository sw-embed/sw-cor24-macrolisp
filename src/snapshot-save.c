/* snapshot-save.c -- Dump prelude state as binary snapshot via UART
 *
 * Build: just build-snapshot-save
 * Run:   just snapshot
 *
 * Outputs a binary blob that can be loaded with:
 *   cor24-run --run build/repl-snapshot.s --load-binary build/prelude.snap@0x080000
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

int main() {
    heap_init();
    gc_init();
    symbol_init();
    string_init();
    eval_init();
    gc_enabled = 1;
    load_prelude();

    /* Force a GC to compact the heap */
    gc_collect();

    /* Write snapshot to UART */
    snapshot_save();

    halt();
    return 0;
}
