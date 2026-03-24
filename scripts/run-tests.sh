#!/usr/bin/env bash
# Run the tml24c test suite with optional verbose output.
# Usage: ./scripts/run-tests.sh         # quiet, pass/fail only
#        ./scripts/run-tests.sh -v      # show full emulator output
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"

if [[ "${1:-}" == "-v" ]]; then
    just build
    cor24-run --run build/tml24c.s --speed 0 -n 10000000 2>&1 | \
        grep -E '^\[UART TX' | \
        python3 scripts/extract-uart.py
else
    just test
fi
