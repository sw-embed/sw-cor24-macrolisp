#!/usr/bin/env bash
# Interactive REPL for tml24c on the COR24 emulator.
# Uses cor24-emu --terminal to bridge stdin/stdout to UART.
# Ctrl-] to exit.
#
# Usage: ./scripts/repl.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"
just build-standard
exec cor24-emu --lgo build/repl-standard.lgo --terminal --echo --speed 0
