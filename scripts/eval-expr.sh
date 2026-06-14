#!/usr/bin/env bash
# Evaluate a single Lisp expression on the COR24 emulator.
# Usage: ./scripts/eval-expr.sh '(+ 1 2)'
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

[[ $# -lt 1 ]] && { echo "Usage: $0 '<expression>'"; exit 1; }

cd "$PROJECT_DIR"
just build-standard

echo "$*" | cor24-emu --lgo build/repl-standard.lgo --uart-file /dev/stdin --quiet --speed 0 -n 200000000 2>/dev/null
