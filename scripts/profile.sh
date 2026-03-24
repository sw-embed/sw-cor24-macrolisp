#!/usr/bin/env bash
# Profile instruction count for a .l24 file or expression.
# Usage: ./scripts/profile.sh examples/demo.l24
#        ./scripts/profile.sh -e '(fact 10)'
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"
make -s build/repl.s

if [[ "${1:-}" == "-e" ]]; then
    shift
    INPUT="$*"
else
    FILE="${1:?Usage: $0 <file.l24> or $0 -e '<expr>'}"
    [[ ! -f "$FILE" ]] && { echo "Error: $FILE not found"; exit 1; }
    INPUT=$(sed '/^;;/d' "$FILE")
fi

OUTPUT=$(echo "$INPUT" | cor24-run --run build/repl.s --terminal --speed 0 -n 10000000 2>&1)

echo "$OUTPUT" | grep -v -E '^Assembled |Executed [0-9]+ instructions' | sed 's/^[> ]*//' | sed '/^$/d'
echo
echo "--- Profile ---"
echo "$OUTPUT" | grep -E '^Assembled |Executed [0-9]+ instructions' || true
