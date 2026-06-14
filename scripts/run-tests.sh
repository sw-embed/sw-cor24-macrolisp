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

    # --- Regression checks (prelude/stack bugs that must not return) ---
    echo "Running regression checks..."

    # lazy.l24 must terminate with correct output at the DEFAULT 3 KB EBR stack.
    # Regression: lazy-take was non-tail-recursive and silently overflowed the
    # C-stack at n=3 (truncated output, no result); it is now tail-recursive.
    lazy_out="$(just eval-full demos/lazy.l24 2>&1)"
    if echo "$lazy_out" | grep -q '(0 1 2 3 4 5 6 7)' \
       && echo "$lazy_out" | grep -q '(0 1 1 2)' \
       && echo "$lazy_out" | grep -q 'Bye\.'; then
        echo "lazy ok"
    else
        echo "FAIL: demos/lazy.l24 did not produce expected output at the 3 KB default stack:"
        echo "$lazy_out"
        exit 1
    fi

    # eval-fullbig (full prelude @ 8 KB EBR) must run past init. Regression: the
    # old SRAM-relocation _start patch tripped cor24-emu's stack guard and halted
    # after 2 instructions; fullbig is now the 8 KB-EBR tier.
    fullbig_out="$(just eval-fullbig demos/lazy.l24 2>&1)"
    if echo "$fullbig_out" | grep -q '(0 1 1 2)' \
       && echo "$fullbig_out" | grep -q 'Bye\.'; then
        echo "fullbig ok"
    else
        echo "FAIL: 'just eval-fullbig' halted before producing output (past-init regression):"
        echo "$fullbig_out"
        exit 1
    fi

    echo "All regression checks passed."
fi
