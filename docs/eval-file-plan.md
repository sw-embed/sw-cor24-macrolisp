# Evaluate Lisp Files from the Command Line

## Usage

```bash
make eval FILE=examples/demo.l24
```

## How It Works

1. The `.l24` file is preprocessed (comments stripped, lines joined with `\n` escapes)
2. Content is passed to `cor24-run` via `--uart-input`
3. The emulator's FIFO-drain mechanism feeds bytes to UART RX one at a time as the program reads them
4. The tml24c REPL evaluates each line and prints results to UART TX
5. `scripts/extract-uart.py` extracts clean output from the `[UART TX]` log

## File Extension

`.l24` — Lisp for COR24.

## Standard Prelude

The REPL build (`src/repl.c`) loads a prelude before accepting input. Available functions:

**List operations:** `map`, `filter`, `foldr`, `length`, `append`, `reverse`, `nth`
**List accessors:** `cadr`, `caddr`, `caar`, `cdar`
**Comparison:** `>`, `>=`, `<=`
**Numeric:** `zero?`, `positive?`, `negative?`, `abs`, `min`, `max`
**Macros:** `when`, `unless`

## Example

```lisp
;; examples/demo.l24
(define square (lambda (n) (* n n)))
(map square (list 1 2 3 4 5))
(filter positive? (list -3 -1 0 1 4))
(reverse (list 1 2 3))
```

Output:
```
#<obj>
(1 4 9 16 25)
(1 4)
(3 2 1)
```

## Build Targets

| Target | Description |
|--------|-------------|
| `make run` | Launch REPL (blocks waiting for UART input) |
| `make eval FILE=x.l24` | Evaluate a file and print results |
| `make test` | Run all test suites |
| `make` | Build test binary only |
