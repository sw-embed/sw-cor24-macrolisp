#!/usr/bin/env bash
# Profile instruction count for a .l24 file or expression.
# Usage: ./scripts/profile.sh examples/demo.l24
#        ./scripts/profile.sh -e '(fact 10)'
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"
just build-standard

if [[ "${1:-}" == "-e" ]]; then
    shift
    INPUT="$*"
else
    FILE="${1:?Usage: $0 <file.l24> or $0 -e '<expr>'}"
    [[ ! -f "$FILE" ]] && { echo "Error: $FILE not found"; exit 1; }
    INPUT=$(grep -v '^;;' "$FILE")
fi

# --quiet sends program output to stdout and emulator diagnostics (the
# "Executed N instructions" line we profile on) to stderr.
mkdir -p build
RESULT=$(echo "$INPUT" | cor24-emu --lgo build/repl-standard.lgo --uart-file /dev/stdin \
    --quiet --speed 0 -n 200000000 2>build/profile.stderr)

echo "$RESULT"
echo
echo "--- Profile ---"
grep -E 'Executed [0-9]+ instructions' build/profile.stderr || true
