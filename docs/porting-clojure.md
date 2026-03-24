# Porting Clojure to tml24c

## Overview

tml24c supports many Clojure idioms despite being a minimal Lisp on a 24-bit bare-metal target. This guide maps Clojure features to their tml24c equivalents, using the 99 Bottles of Beer demos as worked examples.

## Feature Mapping

### Core Forms

| Clojure | tml24c | Notes |
|---------|--------|-------|
| `defn` | `(define f (lambda ...))` | No multi-arity; use `cond` on arg count |
| `defn-` | `(define f (lambda ...))` | No private/public distinction |
| `defmacro` | `(defmacro name (params) body)` | Same semantics, unhygienic |
| `let` | `(let ((x 1) (y 2)) body)` | Macro, single body expression |
| `letfn` | Top-level `define` | No local mutual recursion scope |
| `do` | `(begin ...)` | Same semantics, TCO on last expression |
| `if` | `(if cond then else)` | Same, TCO on both branches |
| `cond` | `(cond (test expr) ... (t default))` | Macro, uses `t` not `:else` |
| `when`/`unless` | `(when c body)` / `(unless c body)` | Macros |
| `and`/`or` | `(and a b)` / `(or a b)` | Two-arg macros (not variadic) |
| `fn` / `#()` | `(lambda (args) body)` | No reader shorthand |
| `loop`/`recur` | Tail recursion | TCO handles this automatically |

### Functions and Data

| Clojure | tml24c | Notes |
|---------|--------|-------|
| `str` | `(str ...)` | Variadic, but expensive — prefer `display` for output |
| `println` | `(display ...) (newline)` | No multi-arg println |
| `print` | `(print x)` / `(display x)` | `print` shows Lisp repr, `display` shows string contents |
| `apply` | `(apply f args-list)` | Primitive |
| `map`/`filter`/`reduce` | Same names | In prelude |
| `every?`/`some` | Same names | In prelude |
| `identity` | `(identity x)` | In prelude |
| `complement` | `(complement f)` | In prelude |
| `compose` | `(compose f g)` | In prelude (Clojure uses `comp`) |
| `constantly` | `(constantly x)` | In prelude |
| `trampoline` | `(trampoline f)` | In prelude, uses `fn?` |
| `fn?` | `(fn? x)` | Primitive: checks closure or primitive |
| `pos?`/`neg?`/`zero?` | `(positive? n)` / `(negative? n)` / `(zero? n)` | In prelude |
| `dec` / `inc` | `(- n 1)` / `(+ n 1)` | No shorthand |
| `number?` | `(number? x)` | Primitive |
| `nil?` | `(null? x)` | Primitive |
| `list?` | `(pair? x)` | Checks cons, not proper list |

### Strings

| Clojure | tml24c | Notes |
|---------|--------|-------|
| `str` (concat) | `(str ...)` | Variadic, coerces via `->str` |
| `(str n)` | `(number->string n)` | Explicit conversion |
| `string?` | `(string? x)` | Primitive |
| `count` on strings | `(string-length s)` | Primitive |
| `subs` | `(string-ref s i)` | Returns char code, not substring |
| `str/join` | `(string-append a b)` | Two-arg only |
| `=` on strings | `(string=? a b)` | Dedicated predicate |
| String literals | `"hello"` | 63-char limit, `\n \r \t \\ \"` escapes |

### Collections

| Clojure | tml24c | Notes |
|---------|--------|-------|
| `(list 1 2 3)` | `(list 1 2 3)` | Same |
| Vectors `[1 2 3]` | `(list 1 2 3)` | No vector type |
| `cons` | `(cons x lst)` | Same |
| `first`/`rest` | `(car x)` / `(cdr x)` | Traditional names |
| `nth` | `(nth n lst)` | In prelude |
| `count` | `(length lst)` | In prelude |
| `concat` | `(append a b)` | Two-arg only |
| `reverse` | `(reverse lst)` | In prelude |
| `take`/`drop` | `(take n lst)` / `(drop n lst)` | In prelude |
| `range` | `(range n)` | 0 to n-1; no start/step |
| `flatten` | `(flatten lst)` | In prelude |
| `zip` | `(zip a b)` | In prelude (pairs, not Clojure-style) |
| Keywords (`:foo`) | Symbols (`'foo`) | No keyword type |
| Maps `{:a 1}` | `(list (cons 'a 1))` | Association lists |
| `assoc` (maps) | `(assoc key alist)` | Finds pair by key |
| `get` (maps) | `(get key alist default)` | Value or default |

### I/O and System

| Clojure | tml24c | Notes |
|---------|--------|-------|
| `println` | `(println x)` | Print + newline |
| `print` | `(print x)` | Print without newline |
| `display` | `(display x)` | Print string without quotes |
| `newline` | `(newline)` | Just a newline |
| `System/exit` | `(exit)` | Halts CPU |
| — | `(peek addr)` | Memory-mapped I/O read |
| — | `(poke addr val)` | Memory-mapped I/O write |
| — | `(delay ms)` | Spin-loop delay (calibrated for --speed 500000) |
| — | `(gc)` | Force GC, return free cell count |
| — | `(heap-used)` / `(heap-size)` | Heap introspection |

### Not Available

| Clojure | Why |
|---------|-----|
| `atom`/`swap!`/`deref` | No mutable references |
| `ns`/`require`/`import` | Single namespace |
| Lazy sequences | No lazy evaluation |
| Persistent data structures | No structural sharing |
| `defmulti`/`defmethod` | Future enhancement (see below) |
| Destructuring | No pattern matching in `let`/`lambda` |
| Java interop | No JVM |
| Regex | No regex engine |
| Concurrency (`agent`, `ref`, `future`) | Single-threaded bare metal |

## Performance: `str` vs `display`

Clojure's `str` builds strings efficiently via Java StringBuilder. In tml24c, `str` uses `(reduce string-append "" args)`, creating intermediate strings for each step. Each intermediate allocates from a fixed 2KB string pool and triggers GC.

For output, always prefer `display` (writes directly to UART, zero allocation):

```lisp
;; SLOW: builds 4 intermediate strings, ~12M instructions
(display (str n " bottles" " of beer" suffix))

;; FAST: 4 direct UART writes, ~200 instructions
(display (number->string n)) (display " bottles") (display " of beer") (display suffix)
```

Use `str` only when you need the resulting string as a value.

## System Limits

| Resource | Limit |
|----------|-------|
| Heap cells | 4096 |
| String pool | 2048 bytes (append-only, not reclaimed by GC) |
| String literal length | 63 characters |
| REPL line buffer | 1024 characters |
| GC root stack | 256 entries |
| Symbol table | 256 symbols, 2048 bytes for names |
| Stack (EBR) | ~3 KB |
| SRAM | 1 MB |

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
2. **`letfn` → top-level `define`**: Forward references work because symbols are looked up at call time, not definition time.
3. **`#(even (dec n))` → `(lambda () (even-verse (- n 1)))`**: Explicit `lambda` for thunks.
4. **`trampoline`**: Same concept — in the tml24c prelude. Uses `fn?` to detect thunks.
5. **`str` → `display`**: Write directly to output instead of building strings.
6. **Multi-arity `defn`**: Not supported. Use a wrapper or default argument.
7. **Single-line expressions**: REPL reads one line at a time (1024 chars max).

## Bottles Demo Variants

| File | Clojure Pattern | tml24c Translation | Command |
|------|----------------|-------------------|---------|
| `bottles.clj` | Macro + loop/recur | Macro + TCO recursion | `just demo-bottles` |
| `bottles2.clj` | Mutual recursion + trampoline | Same pattern | `just demo-bottles2` |
| `bottles3.clj` | Multimethods (`defmulti`/`defmethod`) | Not yet translatable | — |
| `bottles4.clj` | Functional (map/interleave/apply str) | map/for-each + display | `just demo-bottles4` |

### bottles3: Multimethods (future)

`bottles3.clj` uses `defmulti`/`defmethod` for dispatch by verse number — different behavior for n>1, n=1, and n=0. Key Clojure features needed:

- `defmulti`/`defmethod`: dispatch on a key function — can be implemented with alist dispatch tables (see `docs/plan.md`)
- `n2N` number-to-English-word map: requires large alists
- `.toLowerCase`: string case conversion — not available

A partial translation would be possible once multimethods are added to the prelude.

### bottles4: Functional list processing

`bottles4.clj` builds lists of verse strings via `map`, `interleave`s them, and `apply str` to concatenate. The tml24c translation uses `map` + `for-each display` instead of building one huge string, because `str` with `reduce` is O(n) intermediate strings per call and string pool pressure makes bulk concatenation impractical.

## Patterns That Work Well

- **Higher-order functions**: `map`, `filter`, `reduce`, `compose`, `complement`, `apply`
- **Closures and currying**: `(define make-adder (lambda (n) (lambda (x) (+ n x))))`
- **Macros with quasiquote**: `` `(if ,cond ,body nil) `` — same power as Clojure macros
- **Trampolining**: Mutual recursion via thunk-returning functions
- **Tail-call optimization**: `begin`, `if`, and closure calls are all TCO'd
- **Variadic functions**: `(lambda args ...)` and `(lambda (a . rest) ...)`
- **Strings**: Literals, append, length, equality, display
- **Association lists**: `assoc`, `get` — replace Clojure maps for small datasets
- **`let`/`cond`/`and`/`or`**: All available as macros
- **Conservative GC**: No manual memory management needed
