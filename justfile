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
    echo "" | {{cor24_run}} --run build/tml24c.s --terminal --speed 0 -n 50000000 2>&1 | \
        grep -E '^(scaffold|reader|eval|gc|compile) ok$' | sort -u > build/test-results.txt
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
    #!/usr/bin/env bash
    grep -v '^;;' "{{file}}" | {{cor24_run}} --run build/repl.s --terminal --speed 0 -n 200000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU'

# Blink D2 LED demo (Ctrl-] to exit)
demo-blink: build-repl
    grep -v '^;;' demos/blink.l24 | {{cor24_run}} --run build/repl.s --terminal --speed 500000

# 99 Bottles of Beer demo
demo-bottles: build-repl
    grep -v '^;;' demos/bottles.l24 | {{cor24_run}} --run build/repl.s --terminal --speed 0 -n 200000000

# 99 Bottles of Beer (trampoline/mutual recursion version)
demo-bottles2: build-repl
    grep -v '^;;' demos/bottles2.l24 | {{cor24_run}} --run build/repl.s --terminal --speed 0 -n 200000000

# 99 Bottles of Beer (functional map/for-each version)
demo-bottles4: build-repl
    grep -v '^;;' demos/bottles4.l24 | {{cor24_run}} --run build/repl.s --terminal --speed 0 -n 200000000

# Clean build artifacts
clean:
    rm -rf build
