# tml24c -- Tiny Macro Lisp for COR24

set quiet

tc24r := "tc24r"
cor24_run := "cor24-run"

# === Build targets ===

build:
    mkdir -p build
    {{tc24r}} src/main.c -o build/tml24c.s

build-minimal:
    mkdir -p build
    {{tc24r}} src/repl-minimal.c -o build/repl-minimal.s

build-standard:
    mkdir -p build
    {{tc24r}} src/repl-standard.c -o build/repl-standard.s

build-full:
    mkdir -p build
    {{tc24r}} src/repl-full.c -o build/repl-full.s

build-scheme:
    mkdir -p build
    {{tc24r}} src/repl-scheme.c -o build/repl-scheme.s

build-bare:
    mkdir -p build
    {{tc24r}} src/repl-bare.c -o build/repl-bare.s

# === REPL: precompiled preludes ===

# Interactive REPL with minimal prelude
run-minimal: build-minimal
    {{cor24_run}} --run build/repl-minimal.s --terminal --echo --speed 0

# Interactive REPL with standard prelude (default)
run: build-standard
    {{cor24_run}} --run build/repl-standard.s --terminal --echo --speed 0

# Interactive REPL with Scheme prelude
run-scheme: build-scheme
    {{cor24_run}} --run build/repl-scheme.s --terminal --echo --speed 0

# Interactive REPL with full prelude
run-full: build-full
    {{cor24_run}} --run build/repl-full.s --terminal --echo --speed 0

# === REPL: custom prelude from .l24 file (slow, flexible) ===

# Load a custom .l24 prelude then enter REPL
run-custom prelude: build-bare
    grep -v '^;;' "{{prelude}}" | {{cor24_run}} --run build/repl-bare.s --terminal --echo --speed 0

# === Eval ===

# Evaluate a .l24 file with standard prelude
eval file: build-standard
    #!/usr/bin/env bash
    grep -v '^;;' "{{file}}" | {{cor24_run}} --run build/repl-standard.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU'

# Evaluate with Scheme prelude
eval-scheme file: build-scheme
    #!/usr/bin/env bash
    grep -v '^;;' "{{file}}" | {{cor24_run}} --run build/repl-scheme.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU'

# Evaluate with full prelude
eval-full file: build-full
    #!/usr/bin/env bash
    grep -v '^;;' "{{file}}" | {{cor24_run}} --run build/repl-full.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU'

# Evaluate with custom prelude (slow: prelude loaded via UART)
eval-custom file prelude: build-bare
    #!/usr/bin/env bash
    { grep -v '^;;' "{{prelude}}"; grep -v '^;;' "{{file}}"; } | \
        {{cor24_run}} --run build/repl-bare.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU'

# === Tests ===

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

# === Demos (see docs/demos.md for full list) ===

# Blink LED D2 at 1Hz (Ctrl-] to exit, requires --speed 500000)
demo-blink: build-standard
    grep -v '^;;' demos/blink.l24 | {{cor24_run}} --run build/repl-standard.s --terminal --speed 500000

# 99 Bottles of Beer — macro + tail recursion (standard prelude)
demo-bottles: build-standard
    grep -v '^;;' demos/bottles.l24 | {{cor24_run}} --run build/repl-standard.s --terminal --speed 0 -n 500000000

# 99 Bottles — trampoline mutual recursion (full prelude)
demo-bottles2: build-full
    grep -v '^;;' demos/bottles2.l24 | {{cor24_run}} --run build/repl-full.s --terminal --speed 0 -n 500000000

# 99 Bottles — functional map/for-each (standard prelude)
demo-bottles4: build-standard
    grep -v '^;;' demos/bottles4.l24 | {{cor24_run}} --run build/repl-standard.s --terminal --speed 0 -n 500000000

# List available demos
demos:
    @echo "Available demos (run with: just <name>):"
    @echo ""
    @echo "  just demo-bottles   99 Bottles — macro + tail recursion"
    @echo "  just demo-bottles2  99 Bottles — trampoline mutual recursion (full prelude)"
    @echo "  just demo-bottles4  99 Bottles — functional map/for-each"
    @echo "  just demo-blink     Blink LED D2 (Ctrl-] to exit)"
    @echo ""
    @echo "Other demos (run with: just eval demos/<name>.l24):"
    @echo ""
    @echo "  quasiquote   tco          variadic     strings      macros"
    @echo "  anaphora     mutation     threading    lazy         utilities"
    @echo "  multiline    metaprogramming           errors       fixedpoint"
    @echo ""
    @echo "  just eval-full demos/lazy.l24        (full prelude demos)"
    @echo "  just eval-scheme demos/scheme.l24    (scheme prelude)"
    @echo ""
    @echo "See docs/demos.md for details."

# === Clean ===

clean:
    rm -rf build
