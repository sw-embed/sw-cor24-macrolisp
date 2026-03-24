# Porting Clojure to tml24c

## Overview

tml24c supports many Clojure idioms despite being a minimal Lisp on a 24-bit bare-metal target. This guide maps Clojure features to their tml24c equivalents, using the 99 Bottles of Beer demo as a worked example.

## Feature Mapping

| Clojure | tml24c | Notes |
|---------|--------|-------|
| `defn` | `(define f (lambda ...))` | No multi-arity; use `cond` on arg count |
| `defn-` | `(define f (lambda ...))` | No private/public distinction |
| `defmacro` | `(defmacro name (params) body)` | Same semantics, unhygienic |
| `let` | `(let ((x 1) (y 2)) body)` | Macro, expands to lambda application |
| `letfn` | Top-level `define` | No local mutual recursion scope |
| `do` | `(begin ...)` | Same semantics |
| `str` | `(str ...)` | Variadic, but expensive on COR24 |
| `println` | `(display ...) (newline)` | No multi-arg println |
| `pos?` | `(positive? n)` | In prelude |
| `dec` / `inc` | `(- n 1)` / `(+ n 1)` | No shorthand |
| `#(f x)` | `(lambda () (f x))` | No reader shorthand for thunks |
| `trampoline` | `(trampoline f)` | In prelude, uses `fn?` |
| `fn?` | `(fn? x)` | Primitive: checks closure or primitive |
| `loop`/`recur` | Tail recursion | TCO handles this automatically |
| `cond` | `(cond (test expr) ... (t default))` | Macro, uses `t` not `:else` |
| `and`/`or` | `(and a b)` / `(or a b)` | Two-arg macros |
| `when`/`unless` | `(when c body)` / `(unless c body)` | Macros |
| `map`/`filter`/`reduce` | Same names | In prelude |
| `every?`/`some` | Same names | In prelude |
| `assoc` (maps) | `(assoc key alist)` | Association lists, not hash maps |
| Keywords (`:foo`) | Symbols (`'foo`) | No keyword type |
| Vectors (`[1 2 3]`) | Lists (`(list 1 2 3)`) | No vector type |
| Strings (`"hello"`) | `"hello"` | 63-char limit, immutable |
| `atom`/`swap!`/`deref` | Not available | No mutable references |
| `ns`/`require`/`import` | Not available | Single namespace |

## Performance Considerations

**`str` vs `display`:** Clojure's `str` builds a string in memory. On COR24, `string-append` allocates from a 2KB pool and each call creates GC pressure. For output, use `display` (writes directly to UART) instead of building strings:

```clojure
;; Clojure
(println (str n " bottle" (if (= 1 n) "" "s")))
```

```lisp
;; tml24c — avoid str for output, use display
(display (number->string n))
(if (= 1 n) (display " bottle") (display " bottles"))
```

Use `str` only when you need the resulting string as a value (e.g., to store or pass to another function).

**Instruction budget:** Each Lisp operation compiles to many COR24 instructions. A function call with argument eval costs ~100-500 instructions. String operations cost more due to GC. Budget ~10M instructions for simple programs, ~200M for complex ones.

## Worked Example: 99 Bottles of Beer

### Clojure version (`demos/bottles2.clj`)

```clojure
(defn- bottles [n]
  (str n " bottle" (if (= 1 n) "" "s")))
(letfn [(odd [n]
          (if (pos? n)
            (do (println (bottles n) "of beer on the wall,"
                         (bottles n) "of beer.")
              #(even (dec n)))
            nil))
        (even [n]
          (do (println "Take one down and pass it around,"
                       (bottles n) "of beer on the wall.\n")
            (if (pos? n) #(odd n) nil)))]
  (defn sing
    ([] (sing 99))
    ([n] (trampoline #(odd n)))))
```

### tml24c translation (`demos/bottles2.l24`)

```lisp
;; bottles: display "N bottle(s)" — uses display not str
(define bottles (lambda (n)
  (begin (display (number->string n))
    (if (= 1 n) (display " bottle") (display " bottles")))))

;; odd-verse: display verse, return thunk to even-verse
(define odd-verse (lambda (n)
  (if (positive? n)
    (begin
      (bottles n) (display " of beer on the wall, ")
      (bottles n) (display " of beer.\n")
      (lambda () (even-verse (- n 1))))
    nil)))

;; even-verse: display "take one down...", return thunk to odd-verse
(define even-verse (lambda (n)
  (begin
    (display "Take one down and pass it around, ")
    (bottles n) (display " of beer on the wall.\n\n")
    (if (positive? n)
      (lambda () (odd-verse n))
      nil))))

;; sing: trampoline the mutual recursion
(define sing (lambda (n)
  (trampoline (lambda () (odd-verse n)))))

(sing 5)
```

### Translation notes

1. **`defn-` → `define` + `lambda`**: No private scope, all definitions are global.
2. **`letfn` → top-level `define`**: Clojure's `letfn` provides local mutual recursion. In tml24c, `odd-verse` and `even-verse` are defined at top level. Forward references work because they're looked up at call time, not definition time.
3. **`#(even (dec n))` → `(lambda () (even-verse (- n 1)))`**: Clojure's `#()` shorthand for thunks becomes explicit `lambda`.
4. **`trampoline`**: Same concept — built into the tml24c prelude. Uses `fn?` to detect thunks.
5. **`str` → `display`**: Clojure builds strings; tml24c writes directly to output for performance.
6. **Multi-arity `defn`**: Clojure's `([] (sing 99))` default not supported. Use a wrapper or default argument.
7. **Single-line constraint**: REPL reads one line at a time (1024 chars max). Multi-line expressions must be joined.

## Patterns That Work Well

- **Higher-order functions**: `map`, `filter`, `reduce`, `compose`, `complement`
- **Closures and currying**: `(define make-adder (lambda (n) (lambda (x) (+ n x))))`
- **Macros with quasiquote**: `` `(if ,cond ,body nil) `` — same as Clojure
- **Trampolining**: Mutual recursion via thunk-returning functions
- **Association lists**: `(assoc key alist)`, `(get key alist default)` — replaces Clojure maps

## Performance: `str` vs `display`

Clojure's `str` builds strings in memory via efficient Java StringBuilder. In tml24c, `str` uses `(reduce string-append "" args)`, creating intermediate strings for each step. Each intermediate allocates from a fixed 2KB string pool and triggers GC pressure.

For output, always prefer `display` (writes directly to UART) over building strings with `str`:

```lisp
;; SLOW: builds 4 intermediate strings, ~12M instructions
(display (str n " bottles" " of beer" suffix))

;; FAST: 4 direct UART writes, ~200 instructions
(display (number->string n)) (display " bottles") (display " of beer") (display suffix)
```

Use `str` only when you need the resulting string as a value.

## Bottles Demo Variants

| File | Clojure Pattern | tml24c Translation | Status |
|------|----------------|-------------------|--------|
| `bottles.clj` | Macro + loop/recur | Macro + TCO recursion | `bottles.l24` — works |
| `bottles2.clj` | Mutual recursion + trampoline | Same pattern | `bottles2.l24` — works |
| `bottles3.clj` | Multimethods (`defmulti`/`defmethod`) | Not yet translatable | See below |
| `bottles4.clj` | Functional (map/interleave/apply str) | map/for-each + display | `bottles4.l24` — works |

### bottles3: Multimethods (future)

`bottles3.clj` uses `defmulti`/`defmethod` for dispatch by verse number — different behavior for n>1, n=1, and n=0. This requires multimethods which are documented as a future enhancement in `docs/plan.md`. Key Clojure features needed:

- `defmulti`/`defmethod`: dispatch on a key function — can be implemented with alist dispatch tables
- `n2N` number-to-English-word map: requires hash maps or large alists
- `.toLowerCase`: string case conversion — not available

A partial translation would be possible once multimethods are added to the prelude.

### bottles4: Functional list processing

`bottles4.clj` builds lists of verse strings via `map`, `interleave`s them, and `apply str` to concatenate. The tml24c translation uses `map` + `for-each display` instead of building one huge string, because:
- `str` with `reduce` creates O(n) intermediate strings per call
- String pool (2KB) and GC pressure make bulk string building impractical
- `display` writes directly to UART with zero allocation

## Patterns That Don't Translate

- **Lazy sequences**: No lazy evaluation
- **Persistent data structures**: Lists are mutable via cons, no structural sharing
- **Protocols/multimethods**: Possible via alists (see docs/plan.md) but verbose
- **Concurrency** (`atom`, `agent`, `ref`): Single-threaded bare metal
- **Java interop**: No JVM
- **Destructuring**: No pattern matching in `let` or `lambda` params
