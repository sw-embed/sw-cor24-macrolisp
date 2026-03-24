TC24R = tc24r
COR24_RUN = cor24-run
SRC = src/main.c
ASM = build/tml24c.s

all: $(ASM)

$(ASM): $(SRC) src/tml.h src/io.h src/heap.h src/symbol.h src/print.h src/read.h src/eval.h src/gc.h
	@mkdir -p build
	$(TC24R) $(SRC) -o $(ASM)

run: $(ASM)
	$(COR24_RUN) --run $(ASM)

clean:
	rm -rf build
