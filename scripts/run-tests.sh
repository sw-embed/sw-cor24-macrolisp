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
    # Full emulator output, including the decoded "UART output:" summary line.
    # -u '\x04' feeds an EOF so the scaffold halts cleanly after its self-tests.
    cor24-emu --lgo build/tml24c.lgo -u '\x04' --speed 0 -n 200000000 2>&1
else
    just test
fi
