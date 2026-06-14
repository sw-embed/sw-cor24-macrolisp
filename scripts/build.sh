#!/usr/bin/env bash
# build.sh — Build and test sw-cor24-macrolisp
set -euo pipefail
cd "$(dirname "$0")/.."

echo "=== sw-cor24-macrolisp — Tiny Macro Lisp for COR24 ==="

# Verify the COR24 toolchain is available: tc24r (C -> .s),
# cor24-asm (.s -> .lgo), cor24-emu (run .lgo).
if ! command -v tc24r &>/dev/null; then
  echo "ERROR: tc24r not found. Install the COR24 toolchain (see onboarding)."
  exit 1
fi
if ! command -v cor24-asm &>/dev/null; then
  echo "ERROR: cor24-asm not found. Install the COR24 toolchain (see onboarding)."
  exit 1
fi
if ! command -v cor24-emu &>/dev/null; then
  echo "ERROR: cor24-emu not found. Install the COR24 toolchain (see onboarding)."
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
