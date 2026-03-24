TC24R = tc24r
COR24_RUN = cor24-run
SRC = src/main.c
ASM = build/tml24c.s

all: $(ASM)

$(ASM): $(SRC) src/tml.h src/io.h src/heap.h src/symbol.h src/print.h src/read.h src/eval.h src/gc.h src/compile.h
	@mkdir -p build
	$(TC24R) $(SRC) -o $(ASM)

run: $(ASM)
	$(COR24_RUN) --run $(ASM) --speed 500000

test: $(ASM)
	@echo "Running tml24c tests..."
	@$(COR24_RUN) --run $(ASM) --speed 500000 -n 2000000 2>&1 | \
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

clean:
	rm -rf build
