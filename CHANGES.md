# Changelog

## 2026-04-24

- Added `demos/power-dsl.l24` — `power` rewritten in a glyph macro DSL using Unicode (•, →, ←, ┌, │, └). Demonstrates that the reader's symbol-byte rule (`src/read.h:26`) accepts UTF-8 multibyte glyphs as ordinary symbols, so they can fill positional macro params as visual scaffolding

## 2026-04-23

- Added `demos/infix.l24` — fake infix syntax via macros (binary `$`, variadic `infix` with left-to-right associativity)
- Added `demos/fuzzy-eq.l24` — `if~=` DSL macro for `|a - b| <= e` comparisons with keyword-symbol scaffolding
- Added `demos/percent-tol.l24` — `if-close?` percentage-tolerance comparison using integer arithmetic (`100*|α-β| <= ρ*max(|α|,|β|)`) with gensym hygiene for single evaluation
- Added `demos/power.l24` — tail-recursive integer exponentiation `Power N E`; documents the 22-bit fixnum overflow
- Added 33 test cases in `src/main.c` covering the three macro DSLs and `Power` (exercised on every `just test` run)
- Registered the four new demos in `docs/demos.md`

## Fork Migration (2026-03-30)

Forked from [sw-vibe-coding/tml24c](https://github.com/sw-vibe-coding/tml24c)
to [sw-embed/sw-cor24-macrolisp](https://github.com/sw-embed/sw-cor24-macrolisp)
as part of the COR24 ecosystem consolidation.

### Changes

- Renamed project references from `tml24c` to `sw-cor24-macrolisp`
- Updated README.md with ecosystem links and provenance
- Updated CLAUDE.md — removed legacy agentrail protocol, added provenance
- Added `scripts/build.sh` for unified build/test entry point
- Removed legacy `.agentrail/` session data and `.claude/` settings
- Updated dependency references to new sw-embed repo names
