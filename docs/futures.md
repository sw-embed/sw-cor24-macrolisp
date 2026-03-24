# Future Enhancements

## Numeric Types

### Fixed-Point Arithmetic (no C changes)

Use scaled integers: store cents instead of dollars, millivolts instead of volts. The prelude defines helpers for display and arithmetic.

```lisp
;; Store as cents: $12.50 = 1250
(define $ (lambda (cents) cents))
(define $+ (lambda (a b) (+ a b)))
(define $* (lambda (amount factor) (/ (* amount factor) 100)))
(define $display (lambda (cents)
  (begin
    (display (number->string (/ cents 100)))
    (display ".")
    (let ((frac (abs (% cents 100))))
      (when (< frac 10) (display "0"))
      (display (number->string frac))))))
```

**Pros:** Zero C changes, exact for currency/measurement, natural for embedded (sensor readings in mV, etc.)
**Cons:** User must track scale, no automatic promotion.

### Rational Numbers (no C changes)

Represent as `(numerator . denominator)` pairs:

```lisp
(define make-rat (lambda (n d) (let ((g (gcd n d))) (cons (/ n g) (/ d g)))))
(define rat+ (lambda (a b) (make-rat (+ (* (car a) (cdr b)) (* (car b) (cdr a))) (* (cdr a) (cdr b)))))
(define rat-display (lambda (r) (begin (display (number->string (car r))) (display "/") (display (number->string (cdr r))))))
```

**Pros:** Exact fractions, no precision loss. Good for ratios, proportions.
**Cons:** Needs `gcd` (implementable in Lisp). Arithmetic allocates cons cells. 22-bit fixnum limits numerator/denominator to ±2M.

### Software Floating Point (requires C compiler + emulator changes)

Would need `tc24r` and COR24 emulator to support software float operations, then:

| Task | Effort | Notes |
|------|--------|-------|
| New ETYPE_FLOAT extended type | Small | Store float bits in heap cell(s) |
| Reader: `3.14`, `-0.5`, `1e3` | Medium | Detect `.` or `e` in number |
| Printer: float to string | Medium | ~50 lines without printf |
| Arithmetic dispatch | Medium | Fixnum×fixnum, float×float, mixed promotion |
| `float?`, `exact->inexact`, `inexact->exact` | Small | Type predicates and conversion |

**24-bit float** (fits one cell): ~3 decimal digits of precision. Barely useful.
**32-bit float** (spans two cells): ~7 digits. Usable for most purposes but awkward storage.

### BigInt / BigDecimal (large effort)

Arbitrary-precision integers using linked lists of digit chunks:

```
BigInt 123456 → (cons 123 (cons 456 nil))  ; base-1000 digits
```

| Feature | Effort | Memory Cost |
|---------|--------|-------------|
| BigInt (arbitrary precision integer) | Large (~200 lines C) | ~1 cell per 3 digits |
| BigDecimal (arbitrary precision decimal) | Very large | BigInt + scale factor |
| Scheme numeric tower (exact rationals) | Very large | Rational + BigInt |

On a 4096-cell heap, a 10-digit BigInt uses ~4 cells. 100 BigInts = 400 cells (10% of heap). Feasible for small quantities but expensive.

**Recommendation:** Fixed-point and rationals for now (zero C changes). Float only if the C toolchain adds software float support. BigInt/BigDecimal are impractical on COR24.

## Data Structures

### Flat Vectors (medium effort)

Indexed array stored as an extended heap object. Elements in consecutive heap cells.

```lisp
(define v (vector 10 20 30 40))
(vector-ref v 2)    ;; => 30  O(1)
(vector-set! v 2 99) ;; mutate in place
(vector-length v)    ;; => 4
```

| Task | Effort | Notes |
|------|--------|-------|
| ETYPE_VECTOR extended type | Small | Header cell: type + length |
| Allocate N contiguous cells | Medium | Need allocator change for blocks |
| `vector`, `vector-ref`, `vector-set!`, `vector-length` | Small | 4 primitives |
| `vector->list`, `list->vector` | Small | Conversion |
| GC support: mark all cells in vector | Small | Mark header, mark each element |

**Pros:** O(1) random access, cache-friendly, natural for buffers and lookup tables.
**Cons:** Contiguous allocation is hard with mark-sweep GC (fragmentation). Mutation required for useful updates.

**Estimated effort:** ~80 lines of C. Would consume 1 + N cells for an N-element vector.

### Persistent Vectors (large effort, Clojure-style)

Clojure's persistent vectors use a 32-way branching trie (bit-partitioned hash trie):

```
Level 0: root node (up to 32 children)
Level 1: internal nodes (up to 32 children each)
Level 2: leaf nodes (up to 32 elements)
```

A vector of 1000 elements uses ~33 internal nodes + 32 leaves = ~65 cells overhead (plus 1000 element cells). Total: ~1065 cells for 1000 elements.

| Operation | Time | Space |
|-----------|------|-------|
| Lookup `(nth v i)` | O(log32 n) ≈ O(1) for n < 1M | — |
| Update `(assoc v i val)` | O(log32 n) | ~log32(n) new nodes |
| Append `(conj v val)` | O(log32 n) amortized | ~1 new node |
| Structure sharing | ~97% of old tree retained | Only path from root to change |

**Implementation needs:**
- 32-wide array nodes (32 cells each = expensive on 4096-cell heap)
- Bitmap-indexed nodes for sparse levels (complex)
- Tail optimization (Clojure stores last 32 elements flat)
- ~500 lines of C

**Practical on COR24?** Marginally. A 100-element persistent vector needs ~100 element cells + ~10 internal nodes × 32 cells = ~420 cells (10% of heap). Useful for small persistent structures, not for large data.

### Hash Maps (large effort)

Clojure-style persistent hash maps use Hash Array Mapped Tries (HAMTs):

- Hash the key to a 24-bit value
- Use 5 bits per level for trie navigation
- Bitmap-indexed nodes for sparse levels

**Even harder than vectors** because it needs a good hash function for symbols, fixnums, and strings. ~600 lines of C. Lower priority than vectors.

### Recommended Priority

1. **Fixed-point conventions** — prelude only, immediate value
2. **Rational number library** — prelude only, good for teaching
3. **Flat vectors** — medium C effort, immediate utility for buffers/tables
4. **Persistent vectors** — large effort, only if heap is increased
5. **Software floats** — blocked on tc24r/emulator support
6. **Hash maps** — large effort, low priority
7. **BigInt/BigDecimal** — impractical on COR24

## Structural Sharing

Cons lists already share structure naturally:

```lisp
(define a (list 3 4 5))
(define b (cons 2 a))   ;; b shares a's tail
(define c (cons 1 a))   ;; c also shares a's tail
;; a, b, c share the (3 4 5) cells
```

This is the simplest form of persistence. Clojure extends this to vectors and maps via HAMTs (see above). On COR24, the practical path is:

1. **Cons list sharing** — already works, free
2. **Association list "maps"** — already works, O(n) lookup
3. **Flat vectors** — O(1) lookup, mutable, no sharing
4. **Persistent vectors** — O(log n) lookup, immutable, shared — future goal
