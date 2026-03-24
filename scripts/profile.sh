#!/usr/bin/env bash
# Profile instruction count for a .l24 file or expression.
# Usage: ./scripts/profile.sh examples/demo.l24
#        ./scripts/profile.sh -e '(fact 10)'
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"
just build-repl

if [[ "${1:-}" == "-e" ]]; then
    shift
    INPUT="$*"
else
    FILE="${1:?Usage: $0 <file.l24> or $0 -e '<expr>'}"
    [[ ! -f "$FILE" ]] && { echo "Error: $FILE not found"; exit 1; }
    INPUT=$(grep -v '^;;' "$FILE")
fi

OUTPUT=$(echo "$INPUT" | cor24-run --run build/repl.s --terminal --speed 0 -n 200000000 2>&1)

echo "$OUTPUT" | grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU'
echo
echo "--- Profile ---"
echo "$OUTPUT" | grep -E '^Assembled |Executed [0-9]+ instructions' || true
