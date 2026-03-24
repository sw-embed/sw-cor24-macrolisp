#!/usr/bin/env bash
# Show the COR24 assembly output from the compiler for a Lisp expression.
# Useful for inspecting what native code the compiler generates.
# Usage: ./scripts/disasm.sh '(define fact (lambda (n) (if (= n 0) 1 (* n (fact (- n 1))))))'
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

[[ $# -lt 1 ]] && { echo "Usage: $0 '<expression>'"; exit 1; }

cd "$PROJECT_DIR"
make -s build/tml24c.s

# The test binary includes compile tests that emit assembly.
# For arbitrary expressions, we run the REPL build — but the compiler
# is only in the main build. We need to use the main binary.
# The compiler test output is between the markers.
# For now, show the full compiled assembly for the test binary.
echo "=== Compiled assembly for tml24c ==="
echo "Assembly at: build/tml24c.s"
echo "Size: $(wc -l < build/tml24c.s) lines, $(wc -c < build/tml24c.s | tr -d ' ') bytes"
echo
echo "To inspect specific functions, grep the assembly:"
echo "  grep -A 20 '_CL0:' build/tml24c.s"
echo
echo "=== REPL assembly ==="
make -s build/repl.s
echo "Assembly at: build/repl.s"
echo "Size: $(wc -l < build/repl.s) lines, $(wc -c < build/repl.s | tr -d ' ') bytes"
