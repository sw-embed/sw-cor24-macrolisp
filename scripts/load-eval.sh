#!/usr/bin/env bash
# Load and evaluate a .l24 file on the COR24 emulator.
# Usage: ./scripts/load-eval.sh <path.l24> [-n max_instructions] [-v]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

MAX_INSN=10000000
VERBOSE=0

PRELUDE=""

usage() {
    echo "Usage: $0 <file.l24> [-n max_instructions] [-v] [-p prelude.l24]"
    echo
    echo "Options:"
    echo "  -n NUM            Max instructions (default: $MAX_INSN)"
    echo "  -v                Verbose: show assembler info and instruction count"
    echo "  -p prelude.l24    Load a prelude file before the main file"
    echo "  --no-prelude      Skip the built-in C prelude (use bare REPL)"
    exit 1
}

[[ $# -lt 1 ]] && usage
FILE="$1"; shift

while [[ $# -gt 0 ]]; do
    case "$1" in
        -n) MAX_INSN="$2"; shift 2 ;;
        -v) VERBOSE=1; shift ;;
        -p) PRELUDE="$2"; shift 2 ;;
        *)  usage ;;
    esac
done

[[ ! -f "$FILE" ]] && { echo "Error: $FILE not found"; exit 1; }
[[ -n "$PRELUDE" && ! -f "$PRELUDE" ]] && { echo "Error: $PRELUDE not found"; exit 1; }

cd "$PROJECT_DIR"
just build-repl

# Build input: optional prelude + main file
INPUT_CMD="sed '/^;;/d' \"$FILE\""
if [[ -n "$PRELUDE" ]]; then
    INPUT_CMD="{ sed '/^;;/d' \"$PRELUDE\"; sed '/^;;/d' \"$FILE\"; }"
fi

if [[ $VERBOSE -eq 1 ]]; then
    eval "$INPUT_CMD" | cor24-run --run build/repl.s --terminal --speed 0 -n "$MAX_INSN" 2>&1
else
    eval "$INPUT_CMD" | cor24-run --run build/repl.s --terminal --speed 0 -n "$MAX_INSN" 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions' | sed 's/^[> ]*//' | sed '/^$/d'
fi
