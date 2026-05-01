# Changelog

## 2026-05-01

- `prelude-full`: convert `zip` to tail-recursive via `zip-helper` accumulator + `reverse`.
- `prelude-full`: convert `take` to tail-recursive via `take-helper` accumulator + `reverse`.
- `prelude-full`: convert `repeat` to tail-recursive via `repeat-helper` accumulator. All elements identical so cons-order doesn't matter; no reverse needed.
- `prelude-full`: convert `range` to tail-recursive (count-down accumulator). Cuts `demos/functional.l24` peak C-stack from ~3.2 KB to ~2.6 KB — now fits in the 3 KB EBR default. Old form was `(cons i (range-helper (+ i 1) n))` which held a frame per element; new form is `(range-helper (- i 1) (cons i acc))`, no reverse needed since cons-from-the-tail builds in natural order.

## 2026-04-30

- Fix REPL/compiler dropping all but the first form on a line: `(foo 42)(foo 42)` now runs both calls. Each driver was calling `read_str(line)` which only parses one expression — replaced with a drain loop that calls `read_expr` until end-of-buffer. Patched `src/repl-{standard,bare,minimal,full,scheme,snapshot}.c`, `src/main.c`, and `src/compiler.c`.
- README: add standard epilog (Blog/Discord/YouTube links, Copyright, License) matching the `web-sw-cor24-basic` pattern.
- Add `build-fullbig` / `eval-fullbig` diagnostic targets: full prelude with SP relocated to 0x100000 (top of SRAM, ~340 KB headroom) instead of the 3–8 KB EBR. Use to distinguish "C-stack budget too tight" from "unbounded recursion." Implemented as an awk pass that prepends `la r0,1048576; mov sp,r0` to the generated `_start`. Used to confirm `demos/functional.l24` peaks at ~3.2 KB stack (overflows the 3 KB default; fits comfortably in 8 KB).
- Sibling sync: regenerated `web-sw-cor24-macrolisp/asm/repl-*.s` from current C source (picks up the drain-loop fix). Added `StackSize::SixteenKbSram` to the web UI (runtime-patches `_start` to relocate SP to SRAM 0x100000). Set Functional Toolkit demo to default to 16 KB SRAM stack.

## 2026-04-24

- `cd6732b` `demos/fuzzy-eq.l24`: demonstrate the Unicode-glyph variant — add a runnable `(defmacro if≈ …)` section alongside the ASCII `if~=` form, using real math symbols (≈ U+2248, ± U+00B1) for the infix keywords.
- `a6396f3` Add `demos/power-dsl.l24` — `power` rewritten in a glyph macro DSL. Initial form used DOS-style box corners (┌│└) as positional scaffolding; demonstrates that the reader's symbol-byte rule (`src/read.h:26`) accepts UTF-8 multibyte glyphs as ordinary symbols.
- `8985db2` Add `demos/power-dsl-iff.l24` — extended glyph DSL with type annotations (`n : ℤ`) and an `iff e is positive` precondition. The macro strips type triples from the arglist and rewrites the iff-tail into `(assert (positive? e))`; e ≤ 0 raises "assertion failed".
- `73a9152` Fix Power DSL demos: swap DOS corners (┌│└) for Unicode curly-brace pieces (⎧⎨⎩, U+23A7/23A8/23A9); move `iff` inside `loop` (between `times` and the body brace) so it visually tracks the loop control; define `assert` locally in the iff variant so the demo runs on any Standard snapshot (the web UI's cached snapshot predates the prelude's built-in `assert`).
- `99761ce` Refactor Power DSL: the curly brace `⎧⎨⎩` is the macro. `loop`/`iff` live inside the brace on the middle row, and `•` body reduces to the brace expression. Added DSL-parsing-fragility notes (positional binding silently breaks on token reorder).
- `724c0e1` Match updated `docs/fix.txt` layout: relocate the per-step combination (operand, operator) to the `⎩` row; the middle row (`⎨`) carries only the loop control + `iff`. Flattened form: `(• Power (n : ℤ e : ℤ) → (p : ℤ) ← (⎧ 1 ⎨ loop e times iff e is positive ⎩ n *))`.
- `9e28d76` Backfill today's CHANGES.md section to one bullet per commit.
- CLAUDE.md: add a "Changelog discipline" section requiring a `CHANGES.md` entry on every commit, and a "Sibling consumers" note reminding that the web UI must be rebuilt when `demos/*.l24` files change.
- Added `demos/fuzzy-pct.l24` — `(if≈% α ≈ β ± ρ % then X else Y)` infix macro combining fuzzy-eq with percent-tolerance. Unicode and ASCII (`if~=%`) twins; gensym-hygienic so α and β evaluate exactly once. Pure integer math predicate `100·|α-β| ≤ ρ·max(|α|,|β|)`.
- Extended `demos/fuzzy-pct.l24` with `aif≈%` anaphoric variant: `(aif≈% α ≈ β ± ρ %)` binds `it` to the signed delta and yields 0 when close, else `it`. Enables drift-aggregation patterns like `(reduce + 0 (map (lambda (s) (abs (aif≈% s ≈ target ± tol %))) samples))`.

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
