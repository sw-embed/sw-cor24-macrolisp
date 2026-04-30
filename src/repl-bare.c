/* tml24c bare REPL -- no prelude, just the eval loop.
 * Prelude loaded from .l24 files via UART input.
 * Usage: cat prelude/standard.l24 myapp.l24 | cor24-run --run build/repl-bare.s --terminal
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

void repl() {
    char line[1024];
    puts_str("> ");
    while (1) {
        int len = read_line(line, 1024);
        if (len < 0) {
            puts_str("Bye.\n");
            halt();
        }
        if (len == 0) {
            puts_str("> ");
            continue;
        }
        read_ptr = line;
        skip_whitespace();
        while (*read_ptr) {
            int expr = read_expr();
            int result = eval(expr, global_env);
            print_val(result);
            putc_uart('\n');
            skip_whitespace();
        }
        puts_str("> ");
    }
}

int main() {
    heap_init();
    gc_init();
    symbol_init();
    string_init();
    eval_init();
    gc_enabled = 1;
    repl();
    return 0;
}
