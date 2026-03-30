#!/usr/bin/env bash
# build.sh — Build and test sw-cor24-macrolisp
set -euo pipefail
cd "$(dirname "$0")/.."

echo "=== sw-cor24-macrolisp — Tiny Macro Lisp for COR24 ==="

# Verify tc24r and cor24-run are available
if ! command -v tc24r &>/dev/null; then
  echo "ERROR: tc24r not found. Build sw-cor24-tinyc first."
  exit 1
fi
if ! command -v cor24-run &>/dev/null; then
  echo "ERROR: cor24-run not found. Build sw-cor24-emulator first."
  exit 1
fi

# Build
echo ""
echo "--- Building ---"
just build

# Test
echo ""
echo "--- Running test suite ---"
just test

echo ""
echo "=== All tests passed ==="
