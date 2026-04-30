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

# Diagnostic build: full prelude with stack relocated to top of SRAM (~340 KB
# headroom vs. EBR's hard 8 KB cap). Use to test whether a demo overflows due
# to C-stack budget rather than an unbounded recursion.
build-fullbig: build-full
    #!/usr/bin/env bash
    set -euo pipefail
    # Patch _start to load sp = 0x100000 (top of SRAM, grows down) before calling _main
    awk '/^_start:$/ {print; print "        la      r0,1048576"; print "        mov     sp,r0"; next} {print}' \
        build/repl-full.s > build/repl-fullbig.s

build-scheme:
    mkdir -p build
    {{tc24r}} src/repl-scheme.c -o build/repl-scheme.s

build-bare:
    mkdir -p build
    {{tc24r}} src/repl-bare.c -o build/repl-bare.s

build-compiler:
    mkdir -p build
    {{tc24r}} src/compiler.c -o build/compiler.s

build-snapshot-save:
    mkdir -p build
    {{tc24r}} src/snapshot-save.c -o build/snapshot-save.s

build-snapshot:
    mkdir -p build
    {{tc24r}} src/repl-snapshot.c -o build/repl-snapshot.s

# Generate prelude snapshot (binary blob)
snapshot: build-snapshot-save
    #!/usr/bin/env bash
    set -euo pipefail
    echo "Generating prelude snapshot..."
    echo "" | {{cor24_run}} --run build/snapshot-save.s --terminal --speed 0 -n 50000000 2>&1 > build/prelude.snap.raw
    python3 scripts/extract-snapshot.py build/prelude.snap.raw build/prelude.snap

# === REPL: precompiled preludes ===

# Interactive REPL with snapshot-accelerated standard prelude
run-fast: build-snapshot snapshot
    {{cor24_run}} --run build/repl-snapshot.s --load-binary build/prelude.snap@0x080000 --terminal --echo --speed 0

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

# Evaluate with full prelude + relocated SRAM stack (diagnostic). If a demo
# completes here but overflows under `eval-full`, the issue is C-stack budget,
# not unbounded recursion.
eval-fullbig file: build-fullbig
    #!/usr/bin/env bash
    grep -v '^;;' "{{file}}" | {{cor24_run}} --run build/repl-fullbig.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU'

# Evaluate with custom prelude (slow: prelude loaded via UART)
eval-custom file prelude: build-bare
    #!/usr/bin/env bash
    { grep -v '^;;' "{{prelude}}"; grep -v '^;;' "{{file}}"; } | \
        {{cor24_run}} --run build/repl-bare.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU'

# === Compile (Lisp to COR24 assembly) ===

# Compile a .l24 file to .s assembly (output to stdout)
compile file: build-compiler
    #!/usr/bin/env bash
    set -euo pipefail
    { grep -v '^;;' "{{file}}"; printf '\004'; } | \
        {{cor24_run}} --run build/compiler.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU' > build/compiled.s
    cat build/compiled.s

# Compile a .l24 file and run the resulting .s
run-compiled file: build-compiler
    #!/usr/bin/env bash
    set -euo pipefail
    { grep -v '^;;' "{{file}}"; printf '\004'; } | \
        {{cor24_run}} --run build/compiler.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU' > build/compiled.s
    {{cor24_run}} --run build/compiled.s --speed 0 -n 10000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU'

# Compile and run with UART input (for interrupt demos)
run-compiled-uart file input: build-compiler
    #!/usr/bin/env bash
    set -euo pipefail
    { grep -v '^;;' "{{file}}"; printf '\004'; } | \
        {{cor24_run}} --run build/compiler.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU' > build/compiled.s
    {{cor24_run}} --run build/compiled.s --speed 0 -n 10000000 -u "{{input}}" 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU'

# === Tests ===

# Test compiled asm output assembles and runs correctly
test-asm: build-compiler
    #!/usr/bin/env bash
    set -euo pipefail
    echo "Running asm compiler tests..."
    # Compile isr-echo demo
    { grep -v '^;;' demos/isr-echo.l24; printf '\004'; } | \
        {{cor24_run}} --run build/compiler.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU' > build/test-asm.s
    # Verify it assembles and runs (echo "AB" back)
    output=$({{cor24_run}} --run build/test-asm.s --speed 0 -n 10000000 -u "AB" 2>&1 | \
        grep 'UART output:' | sed 's/.*UART output: //')
    if [ "$output" = "AB" ]; then
        echo "asm ok"
    else
        echo "FAIL: expected 'AB', got '$output'"
        exit 1
    fi

test: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "Running tml24c tests..."
    echo "" | {{cor24_run}} --run build/tml24c.s --terminal --speed 0 -n 100000000 2>&1 | \
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

# Reduce, fold, and aggregate patterns (full prelude)
demo-reduce: build-full
    grep -v '^;;' demos/reduce.l24 | {{cor24_run}} --run build/repl-full.s --terminal --speed 0 -n 500000000

# Escape continuations and error handling
demo-continuations: build-standard
    grep -v '^;;' demos/continuations.l24 | {{cor24_run}} --run build/repl-standard.s --terminal --speed 0 -n 500000000

# Interrupt-driven UART echo (compiled Lisp + inline asm)
demo-isr-echo: build-compiler
    just run-compiled-uart demos/isr-echo.l24 "Hello, COR24!"

# Multi-module: Lisp dispatches to 5 .s service modules
demo-multi: build-compiler
    #!/usr/bin/env bash
    set -euo pipefail
    # Assemble service modules at fixed addresses
    {{cor24_run}} --assemble demos/multi/uart.s  build/uart.bin  build/uart.lst  --base-addr 0x1000
    {{cor24_run}} --assemble demos/multi/spi.s   build/spi.bin   build/spi.lst   --base-addr 0x2000
    {{cor24_run}} --assemble demos/multi/i2c.s   build/i2c.bin   build/i2c.lst   --base-addr 0x3000
    {{cor24_run}} --assemble demos/multi/gpio.s  build/gpio.bin  build/gpio.lst  --base-addr 0x4000
    {{cor24_run}} --assemble demos/multi/timer.s build/timer.bin build/timer.lst --base-addr 0x5000
    # Compile main.l24 to .s
    { grep -v '^;;' demos/multi/main.l24; printf '\004'; } | \
        {{cor24_run}} --run build/compiler.s --terminal --speed 0 -n 500000000 2>&1 | \
        grep -v -E '^Assembled |Executed [0-9]+ instructions|^\[CPU' > build/main.s
    # Run with all modules loaded
    {{cor24_run}} --run build/main.s \
        --load-binary build/uart.bin@0x1000 \
        --load-binary build/spi.bin@0x2000 \
        --load-binary build/i2c.bin@0x3000 \
        --load-binary build/gpio.bin@0x4000 \
        --load-binary build/timer.bin@0x5000 \
        --speed 0 -n 10000000 2>&1 | \
        grep -v -E '^Assembled |Loaded |Executed [0-9]+ instructions|^\[CPU'

# List available demos
demos:
    @echo "Available demos (run with: just <name>):"
    @echo ""
    @echo "  just demo-bottles        99 Bottles — macro + tail recursion"
    @echo "  just demo-bottles2       99 Bottles — trampoline mutual recursion (full prelude)"
    @echo "  just demo-bottles4       99 Bottles — functional map/for-each"
    @echo "  just demo-continuations  Escape continuations and error handling"
    @echo "  just demo-blink          Blink LED D2 (Ctrl-] to exit)"
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
