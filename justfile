# tml24c -- Tiny Macro Lisp for COR24

set quiet

tc24r := "tc24r"
cor24_run := "cor24-run"

# Build the test binary (with compiler + test suite)
build:
    mkdir -p build
    {{tc24r}} src/main.c -o build/tml24c.s

# Build the REPL binary (prelude + interactive eval, no tests)
build-repl:
    mkdir -p build
    {{tc24r}} src/repl.c -o build/repl.s

# Interactive REPL (Ctrl-] to exit)
run: build-repl
    {{cor24_run}} --run build/repl.s --terminal --echo --speed 0

# Run all test suites
test: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "Running tml24c tests..."
    {{cor24_run}} --run build/tml24c.s --speed 0 -n 10000000 2>&1 | \
        grep -E '^(scaffold|reader|eval|gc|compile) ok$' | sort > build/test-results.txt
    expected=5
    got=$(wc -l < build/test-results.txt | tr -d ' ')
    if [ "$got" -eq "$expected" ]; then
        echo "All $expected test suites passed."
        cat build/test-results.txt
    else
        echo "FAIL: $got/$expected test suites passed:"
        cat build/test-results.txt
        exit 1
    fi

# Evaluate a .l24 file
eval file: build-repl
    sed '/^;;/d' "{{file}}" | {{cor24_run}} --run build/repl.s --terminal --speed 0 -n 10000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions' | sed 's/^[> ]*//;/^$/d'

# Blink D2 LED demo (Ctrl-] to exit)
demo-blink: build-repl
    sed '/^;;/d' demos/blink.l24 | {{cor24_run}} --run build/repl.s --terminal --speed 500000

# Clean build artifacts
clean:
    rm -rf build
