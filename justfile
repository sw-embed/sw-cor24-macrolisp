# tml24c -- Tiny Macro Lisp for COR24

set quiet

# Toolchain (PATH-resolved). The old all-in-one `cor24-run` is deprecated and
# slated for removal; it bundled an old internal assembler whose output drifted
# from the current tc24r. We assemble with `cor24-asm` (.s -> .lgo) and run with
# `cor24-emu` (--lgo). Batch input is fed via `--uart-file /dev/stdin`, which
# loads the program into the UART RX buffer with flow control (and appends an
# EOF), so no bytes are dropped while the prelude is still loading.
tc24r := "tc24r"
cor24_asm := "cor24-asm"
cor24_emu := "cor24-emu"

# === Build targets ===

build:
    mkdir -p build
    {{tc24r}} src/main.c -o build/tml24c.s
    {{cor24_asm}} build/tml24c.s -o build/tml24c.lgo

build-minimal:
    mkdir -p build
    {{tc24r}} src/repl-minimal.c -o build/repl-minimal.s
    {{cor24_asm}} build/repl-minimal.s -o build/repl-minimal.lgo

build-standard:
    mkdir -p build
    {{tc24r}} src/repl-standard.c -o build/repl-standard.s
    {{cor24_asm}} build/repl-standard.s -o build/repl-standard.lgo

build-full:
    mkdir -p build
    {{tc24r}} src/repl-full.c -o build/repl-full.s
    {{cor24_asm}} build/repl-full.s -o build/repl-full.lgo

build-scheme:
    mkdir -p build
    {{tc24r}} src/repl-scheme.c -o build/repl-scheme.s
    {{cor24_asm}} build/repl-scheme.s -o build/repl-scheme.lgo

build-bare:
    mkdir -p build
    {{tc24r}} src/repl-bare.c -o build/repl-bare.s
    {{cor24_asm}} build/repl-bare.s -o build/repl-bare.lgo

build-compiler:
    mkdir -p build
    {{tc24r}} src/compiler.c -o build/compiler.s
    {{cor24_asm}} build/compiler.s -o build/compiler.lgo

build-snapshot-save:
    mkdir -p build
    {{tc24r}} src/snapshot-save.c -o build/snapshot-save.s
    {{cor24_asm}} build/snapshot-save.s -o build/snapshot-save.lgo

build-snapshot:
    mkdir -p build
    {{tc24r}} src/repl-snapshot.c -o build/repl-snapshot.s
    {{cor24_asm}} build/repl-snapshot.s -o build/repl-snapshot.lgo

# Generate prelude snapshot (binary blob)
snapshot: build-snapshot-save
    #!/usr/bin/env bash
    set -euo pipefail
    echo "Generating prelude snapshot..."
    {{cor24_emu}} --lgo build/snapshot-save.lgo --quiet --speed 0 -n 50000000 2>/dev/null > build/prelude.snap.raw
    python3 scripts/extract-snapshot.py build/prelude.snap.raw build/prelude.snap

# === REPL: precompiled preludes ===

# Interactive REPL with snapshot-accelerated standard prelude
run-fast: build-snapshot snapshot
    {{cor24_emu}} --lgo build/repl-snapshot.lgo --load-binary build/prelude.snap@0x080000 --terminal --echo --speed 0

# Interactive REPL with minimal prelude
run-minimal: build-minimal
    {{cor24_emu}} --lgo build/repl-minimal.lgo --terminal --echo --speed 0

# Interactive REPL with standard prelude (default)
run: build-standard
    {{cor24_emu}} --lgo build/repl-standard.lgo --terminal --echo --speed 0

# Interactive REPL with Scheme prelude
run-scheme: build-scheme
    {{cor24_emu}} --lgo build/repl-scheme.lgo --terminal --echo --speed 0

# Interactive REPL with full prelude
run-full: build-full
    {{cor24_emu}} --lgo build/repl-full.lgo --terminal --echo --speed 0

# === REPL: custom prelude from .l24 file (slow, flexible) ===

# Load a custom .l24 prelude then enter REPL
run-custom prelude: build-bare
    grep -v '^;;' "{{prelude}}" | {{cor24_emu}} --lgo build/repl-bare.lgo --uart-file /dev/stdin --echo --speed 0

# === Eval ===

# Evaluate a .l24 file with standard prelude
eval file: build-standard
    #!/usr/bin/env bash
    grep -v '^;;' "{{file}}" | \
        {{cor24_emu}} --lgo build/repl-standard.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null

# Evaluate with Scheme prelude
eval-scheme file: build-scheme
    #!/usr/bin/env bash
    grep -v '^;;' "{{file}}" | \
        {{cor24_emu}} --lgo build/repl-scheme.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null

# Evaluate with full prelude
eval-full file: build-full
    #!/usr/bin/env bash
    grep -v '^;;' "{{file}}" | \
        {{cor24_emu}} --lgo build/repl-full.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null

# Evaluate with full prelude at the maximum 8 KB EBR stack (diagnostic). cor24-emu
# caps the stack at 8 KB EBR — there is no SRAM-stack mode — so this is the
# deepest-recursion tier available. If a full-prelude program overflows the 3 KB
# default under `eval-full` but completes here, it needs more C-stack budget; if
# it still overflows here, suspect unbounded (non-tail) recursion.
eval-fullbig file: build-full
    #!/usr/bin/env bash
    grep -v '^;;' "{{file}}" | \
        {{cor24_emu}} --lgo build/repl-full.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 --stack-kilobytes 8 2>/dev/null

# Evaluate with custom prelude (slow: prelude loaded via UART)
eval-custom file prelude: build-bare
    #!/usr/bin/env bash
    { grep -v '^;;' "{{prelude}}"; grep -v '^;;' "{{file}}"; } | \
        {{cor24_emu}} --lgo build/repl-bare.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null

# === Compile (Lisp to COR24 assembly) ===

# Compile a .l24 file to .s assembly (output to stdout)
compile file: build-compiler
    #!/usr/bin/env bash
    set -euo pipefail
    { grep -v '^;;' "{{file}}"; printf '\004'; } | \
        {{cor24_emu}} --lgo build/compiler.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null > build/compiled.s
    cat build/compiled.s

# Compile a .l24 file and run the resulting .s
run-compiled file: build-compiler
    #!/usr/bin/env bash
    set -euo pipefail
    { grep -v '^;;' "{{file}}"; printf '\004'; } | \
        {{cor24_emu}} --lgo build/compiler.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null > build/compiled.s
    {{cor24_asm}} build/compiled.s -o build/compiled.lgo
    {{cor24_emu}} --lgo build/compiled.lgo --quiet --speed 0 -n 10000000 2>/dev/null

# Compile and run with UART input (for interrupt demos)
run-compiled-uart file input: build-compiler
    #!/usr/bin/env bash
    set -euo pipefail
    { grep -v '^;;' "{{file}}"; printf '\004'; } | \
        {{cor24_emu}} --lgo build/compiler.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null > build/compiled.s
    {{cor24_asm}} build/compiled.s -o build/compiled.lgo
    {{cor24_emu}} --lgo build/compiled.lgo --quiet --speed 0 -n 10000000 -u "{{input}}" 2>/dev/null

# === Tests ===

# Test compiled asm output assembles and runs correctly
test-asm: build-compiler
    #!/usr/bin/env bash
    set -euo pipefail
    echo "Running asm compiler tests..."
    # Compile isr-echo demo
    { grep -v '^;;' demos/isr-echo.l24; printf '\004'; } | \
        {{cor24_emu}} --lgo build/compiler.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null > build/test-asm.s
    # Verify it assembles and runs (echo "AB" back)
    {{cor24_asm}} build/test-asm.s -o build/test-asm.lgo
    output=$({{cor24_emu}} --lgo build/test-asm.lgo --speed 0 -n 10000000 -u "AB" 2>&1 | \
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
    {{cor24_emu}} --lgo build/tml24c.lgo -u '\x04' --quiet --speed 0 -n 200000000 2>/dev/null | \
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
    grep -v '^;;' demos/blink.l24 | {{cor24_emu}} --lgo build/repl-standard.lgo --uart-file /dev/stdin --terminal --speed 500000

# 99 Bottles of Beer — macro + tail recursion (standard prelude)
demo-bottles: build-standard
    grep -v '^;;' demos/bottles.l24 | {{cor24_emu}} --lgo build/repl-standard.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null

# 99 Bottles — trampoline mutual recursion (full prelude)
demo-bottles2: build-full
    grep -v '^;;' demos/bottles2.l24 | {{cor24_emu}} --lgo build/repl-full.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null

# 99 Bottles — functional map/for-each (standard prelude)
demo-bottles4: build-standard
    grep -v '^;;' demos/bottles4.l24 | {{cor24_emu}} --lgo build/repl-standard.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null

# Reduce, fold, and aggregate patterns (full prelude)
demo-reduce: build-full
    grep -v '^;;' demos/reduce.l24 | {{cor24_emu}} --lgo build/repl-full.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null

# Escape continuations and error handling
demo-continuations: build-standard
    grep -v '^;;' demos/continuations.l24 | {{cor24_emu}} --lgo build/repl-standard.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null

# Interrupt-driven UART echo (compiled Lisp + inline asm)
demo-isr-echo: build-compiler
    just run-compiled-uart demos/isr-echo.l24 "Hello, COR24!"

# Multi-module: Lisp dispatches to 5 .s service modules
demo-multi: build-compiler
    #!/usr/bin/env bash
    set -euo pipefail
    # Assemble service modules at fixed addresses
    {{cor24_asm}} demos/multi/uart.s  --bin build/uart.bin  --listing build/uart.lst  --base-addr 0x1000
    {{cor24_asm}} demos/multi/spi.s   --bin build/spi.bin   --listing build/spi.lst   --base-addr 0x2000
    {{cor24_asm}} demos/multi/i2c.s   --bin build/i2c.bin   --listing build/i2c.lst   --base-addr 0x3000
    {{cor24_asm}} demos/multi/gpio.s  --bin build/gpio.bin  --listing build/gpio.lst  --base-addr 0x4000
    {{cor24_asm}} demos/multi/timer.s --bin build/timer.bin --listing build/timer.lst --base-addr 0x5000
    # Compile main.l24 to .s
    { grep -v '^;;' demos/multi/main.l24; printf '\004'; } | \
        {{cor24_emu}} --lgo build/compiler.lgo --uart-file /dev/stdin --quiet --speed 0 -n 500000000 2>/dev/null > build/main.s
    # Assemble the compiled main
    {{cor24_asm}} build/main.s -o build/main.lgo
    # Run with all modules loaded
    {{cor24_emu}} --lgo build/main.lgo \
        --load-binary build/uart.bin@0x1000 \
        --load-binary build/spi.bin@0x2000 \
        --load-binary build/i2c.bin@0x3000 \
        --load-binary build/gpio.bin@0x4000 \
        --load-binary build/timer.bin@0x5000 \
        --quiet --speed 0 -n 10000000 2>/dev/null

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
