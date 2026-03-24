#!/usr/bin/env bash
# Evaluate a single Lisp expression on the COR24 emulator.
# Usage: ./scripts/eval-expr.sh '(+ 1 2)'
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

[[ $# -lt 1 ]] && { echo "Usage: $0 '<expression>'"; exit 1; }

cd "$PROJECT_DIR"
make -s build/repl.s

echo "$*" | cor24-run --run build/repl.s --terminal --speed 0 -n 10000000 2>&1 | \
    grep -v -E '^Assembled |Executed [0-9]+ instructions' | sed 's/^[> ]*//' | sed '/^$/d'
