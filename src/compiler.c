/* tml24c compiler driver -- reads .l24 from UART, emits COR24 .s
 *
 * Usage: cat program.l24 | cor24-run --run build/compiler.s --terminal --speed 0
 * Output: COR24 assembly on UART (stdout)
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
#include "compile.h"

int main() {
    heap_init();
    gc_init();
    symbol_init();
    string_init();
    eval_init();
    compile_init();
    gc_enabled = 1;

    /* Read all expressions into a list */
    char line[1024];
    int prog = NIL_VAL;
    int tail = NIL_VAL;

    while (1) {
        int len = read_line(line, 1024);
        if (len < 0) break;
        if (len == 0) continue;
        int expr = read_str(line);
        if (IS_NIL(expr)) continue;
        int cell = cons(expr, NIL_VAL);
        if (IS_NIL(prog)) {
            prog = cell;
        } else {
            heap_cdr[PTR_IDX(tail)] = cell;
        }
        tail = cell;
    }

    if (!IS_NIL(prog)) {
        compile_program(prog);
    }

    halt();
    return 0;
}
