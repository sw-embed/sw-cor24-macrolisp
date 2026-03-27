# Demos

All demos are in the `demos/` directory. Run with `just eval demos/<name>.l24` or the named recipe where available.

## Language Features

| Demo | Features | Command | Description |
|------|----------|---------|-------------|
| `quasiquote.l24` | `` ` ``, `,`, `,@` | `just eval demos/quasiquote.l24` | Quasiquote templates, unquote, splicing, macro code generation |
| `tco.l24` | Tail-call optimization | `just eval demos/tco.l24` | Countdown, sum-accumulator, tail-recursive fibonacci — constant stack |
| `variadic.l24` | Rest parameters | `just eval demos/variadic.l24` | `(lambda args ...)`, `(lambda (a . rest) ...)`, variadic sum |
| `strings.l24` | String type | `just eval demos/strings.l24` | Literals, length, append, equality, display, strings in lists |
| `macros.l24` | let, cond, and, or | `just eval demos/macros.l24` | let bindings, multi-way cond, short-circuit and/or, classify function |
| `anaphora.l24` | Unhygienic macros | `just eval demos/anaphora.l24` | aif, awhen, aand — `it` captures test result for use in body |
| `mutation.l24` | set!, closures over mutable state | `just eval demos/mutation.l24` | Counters, memoized fibonacci, accumulator factory |
| `threading.l24` | `->`, `->>` | `just eval demos/threading.l24` | Thread-first/last pipelines, data transformation |
| `lazy.l24` | Lazy sequences | `just eval demos/lazy.l24` | Infinite naturals, powers of 2, lazy map/filter, lazy fibonacci |
| `utilities.l24` | partial, juxt, doseq, dotimes | `just eval demos/utilities.l24` | Curried functions, multi-result, iteration macros |
| `multibody.l24` | Multi-body lambda/define, named let, do loop | `just eval demos/multibody.l24` | Implicit begin, named let iteration, R7RS do loop |
| `iteration.l24` | Named let, do, symbol?, substring, format | `just eval demos/iteration.l24` | Iteration patterns, fibonacci, string formatting, type dispatch |
| `multiline.l24` | Multi-line input | `just eval demos/multiline.l24` | Multi-line function definitions with paren-depth tracking |
| `metaprogramming.l24` | eval, macroexpand, gensym | `just eval demos/metaprogramming.l24` | Runtime eval, macro introspection, hygienic gensym |
| `continuations.l24` | catch/throw, call/ec, raise, guard, dynamic-wind, unwind-protect | `just demo-continuations` | Non-local exit, escape continuations, error handling, guaranteed cleanup |
| `parameters.l24` | make-parameter, parameterize | `just eval demos/parameters.l24` | Dynamic parameters with scoped rebinding and throw-safe restore |
| `restarts.l24` | with-restart, invoke-restart | `just eval demos/restarts.l24` | CL-inspired restartable conditions, map with error recovery |
| `fixedpoint.l24` | Integer arithmetic | `just eval demos/fixedpoint.l24` | Fixed-point currency/rational arithmetic using scaled integers |
| `functional.l24` | fn?, compose, complement, every?, flatten, zip | `just eval-full demos/functional.l24` | Function combinators, list predicates, flatten/zip utilities |
| `errors.l24` | Error messages | `just eval demos/errors.l24` | Evaluator error behavior: type errors, unbound symbols |
| `scheme.l24` | Scheme prelude | `just eval-scheme demos/scheme.l24` | R7RS-style naming: let*, cond/else, even?/odd?, equal? |

## Applications

| Demo | Features | Command | Description |
|------|----------|---------|-------------|
| `bottles.l24` | Macros, TCO, display | `just demo-bottles` | 99 Bottles of Beer — `bottles` macro for pluralization, tail-recursive sing |
| `bottles2.l24` | Trampoline, mutual recursion | `just demo-bottles2` | 99 Bottles via odd/even mutual recursion with thunk trampolining |
| `bottles4.l24` | map, for-each, functional style | `just demo-bottles4` | 99 Bottles via functional list processing (map verse components, display all) |
| `blink.l24` | I/O, delay, peek/poke | `just demo-blink` | Blink LED D2 at 1Hz via memory-mapped I/O (requires `--speed 500000`) |

## Clojure Reference

| File | Description |
|------|-------------|
| `bottles.clj` | Clojure 99 Bottles — macro + loop/recur |
| `bottles2.clj` | Clojure 99 Bottles — mutual recursion + trampoline |
| `bottles3.clj` | Clojure 99 Bottles — multimethods (not yet translatable) |
| `bottles4.clj` | Clojure 99 Bottles — functional map/interleave/apply-str |

See [docs/porting-clojure.md](porting-clojure.md) for translation guide.
