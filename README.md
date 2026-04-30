# sw-cor24-macrolisp

Tiny Macro Lisp for COR24 — a minimal Lisp-1 with lexical scope, unhygienic defmacro, closures, mark-sweep GC, and tail-call optimization. Compiles to COR24 24-bit RISC assembly.

> Forked from [sw-vibe-coding/tml24c](https://github.com/sw-vibe-coding/tml24c) as part of the [COR24 ecosystem consolidation](https://github.com/sw-embed/sw-cor24-project).

Built on:
- [sw-cor24-x-tinyc](https://github.com/sw-embed/sw-cor24-x-tinyc) — Tiny COR24 C compiler (Rust)
- [sw-cor24-emulator](https://github.com/sw-embed/sw-cor24-emulator) — COR24 assembler and emulator

## Quick Start

```bash
just build       # compile
just test        # run test suite
just run         # interactive REPL (Ctrl-] to exit)
just eval <file> # evaluate a .l24 file
```

## Language

- **Types**: fixnums, symbols, cons pairs, strings, closures, macros, primitives
- **Special forms**: `quote`, `if`, `define`, `set!`, `lambda`, `defmacro`, `begin`, `quasiquote`
- **Standard prelude**: 60+ functions and macros including `map`, `filter`, `reduce`, `let`, `cond`, `and`, `or`, `->`, `->>`, lazy sequences, anaphoric macros, and more
- **Tail-call optimization**: `if`, `begin`, and closure application
- **Conservative GC**: scans C stack for heap pointers — no manual root management
- **Multi-line REPL**: paren-depth tracking across newlines
- **I/O**: `peek`/`poke` for memory-mapped hardware, `display` for UART output

## Demos

See [docs/demos.md](docs/demos.md) for the full list. Highlights:

```bash
just demo-bottles   # 99 Bottles of Beer (macro + tail recursion)
just demo-bottles2  # 99 Bottles (trampoline mutual recursion)
just eval demos/lazy.l24       # infinite sequences
just eval demos/anaphora.l24   # anaphoric macros (aif, awhen)
just eval demos/threading.l24  # -> and ->> pipelines
just eval demos/mutation.l24   # set!, counters, memoization
```

## Documentation

| Document | Description |
|----------|-------------|
| [demos.md](docs/demos.md) | Demo descriptions and commands |
| [porting-clojure.md](docs/porting-clojure.md) | Clojure-to-tml24c feature mapping and translation guide |
| [true-and-nil-patterns.md](docs/true-and-nil-patterns.md) | Truth, nil, and boolean conventions |
| [prelude-choices.md](docs/prelude-choices.md) | Prelude tiers (tiny/standard/full/experimental) |
| [plan.md](docs/plan.md) | Implementation plan and future roadmap |
| [bugs.md](docs/bugs.md) | Known issues |

## Links

- Blog: [Software Wrighter Lab](https://software-wrighter-lab.github.io/)
- Discord: [Join the community](https://discord.com/invite/Ctzk5uHggZ)
- YouTube: [Software Wrighter](https://www.youtube.com/@SoftwareWrighter)

## Copyright

Copyright (c) 2026 Michael A. Wright

## License

MIT -- see [LICENSE](LICENSE) for details.
