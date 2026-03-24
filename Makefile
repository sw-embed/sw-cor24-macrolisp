TC24R = tc24r
COR24_RUN = cor24-run
SRC = src/main.c
REPL_SRC = src/repl.c
ASM = build/tml24c.s
REPL_ASM = build/repl.s
HDRS = src/tml.h src/io.h src/heap.h src/symbol.h src/print.h src/read.h src/eval.h src/gc.h

all: $(ASM)

$(ASM): $(SRC) $(HDRS) src/compile.h
	@mkdir -p build
	$(TC24R) $(SRC) -o $(ASM)

$(REPL_ASM): $(REPL_SRC) $(HDRS)
	@mkdir -p build
	$(TC24R) $(REPL_SRC) -o $(REPL_ASM)

run: $(REPL_ASM)
	$(COR24_RUN) --run $(REPL_ASM) --terminal --echo --speed 0

test: $(ASM)
	@echo "Running tml24c tests..."
	@$(COR24_RUN) --run $(ASM) --speed 0 -n 10000000 2>&1 | \
		grep -E "^(scaffold|reader|eval|gc|compile) ok$$" | sort > build/test-results.txt
	@expected=5; got=$$(wc -l < build/test-results.txt | tr -d ' '); \
	if [ "$$got" -eq "$$expected" ]; then \
		echo "All $$expected test suites passed."; \
		cat build/test-results.txt; \
	else \
		echo "FAIL: $$got/$$expected test suites passed:"; \
		cat build/test-results.txt; \
		exit 1; \
	fi

eval: $(REPL_ASM)
	@test -n "$(FILE)" || { echo "Usage: make eval FILE=examples/demo.l24"; exit 1; }
	@sed '/^;;/d' "$(FILE)" | $(COR24_RUN) --run $(REPL_ASM) --terminal --speed 0 -n 10000000 2>&1 | \
		grep -v -E '^Assembled |Executed [0-9]+ instructions' | sed 's/^[> ]*//;/^$$/d'

demo-blink: $(REPL_ASM)
	@sed '/^;;/d' demos/blink.l24 | $(COR24_RUN) --run $(REPL_ASM) --terminal --speed 500000

clean:
	rm -rf build
