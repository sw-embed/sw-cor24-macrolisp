# Truth, Nil, and Boolean Patterns in tml24c

## Current Implementation

tml24c follows traditional Lisp-1 conventions:

| Value | Meaning | Notes |
|-------|---------|-------|
| `nil` | False, empty list, nothing | Symbol at index 0 |
| `t` | True (canonical) | Symbol at index 1 |
| Everything else | Truthy | Fixnums, strings, pairs, closures |

### What is falsy?

Only `nil` is falsy. Everything else — including `0`, `""`, and `'()` — is truthy.

```lisp
(if nil 'yes 'no)      ;; => no  (nil is false)
(if 0 'yes 'no)        ;; => yes (0 is truthy!)
(if "" 'yes 'no)       ;; => yes (empty string is truthy)
(if '() 'yes 'no)      ;; => no  ('() IS nil)
(if t 'yes 'no)        ;; => yes
(if 42 'yes 'no)       ;; => yes
```

### How nil relates to the empty list

`nil` and `'()` are identical — both are `MAKE_SYMBOL(0)`. This is the traditional Lisp convention (shared by Common Lisp and Emacs Lisp). Clojure separates `nil` from `()`, and Scheme uses `#f` for false.

```lisp
(null? nil)    ;; => t
(null? '())    ;; => t  (same thing)
(eq? nil '())  ;; => t
```

## Comparison with Other Lisps

| Feature | tml24c | Common Lisp | Scheme | Clojure | Emacs Lisp |
|---------|--------|-------------|--------|---------|------------|
| False | `nil` | `nil` | `#f` | `nil`, `false` | `nil` |
| True | `t` | `t` | `#t` | `true` | `t` |
| Empty list | `nil`/`'()` | `nil`/`'()` | `'()` | `()` (not nil) | `nil`/`'()` |
| 0 is falsy? | No | No | No | No | No |
| "" is falsy? | No | No | No | No | No |
| Boolean type? | No (t/nil are symbols) | No | Yes (#t/#f) | Yes (true/false) | No |
| `nil` = `'()`? | Yes | Yes | No | No | Yes |

## Design Rationale

### Why nil = empty list (CL/Elisp style, not Scheme/Clojure)

1. **Simplicity**: One fewer type to implement. `nil` serves as false, empty list, and "nothing."
2. **Pattern compatibility**: `(null? (cdr '(a)))` returning `t` is natural — the list ran out.
3. **Conditional returns**: Functions can return `nil` to mean both "not found" and "empty result."
4. **COR24 efficiency**: Fewer tag checks in the evaluator.

### Why t is not reserved / caveats

`t` is a pre-interned symbol checked before env lookup in eval:
```c
if (expr == T_VAL) return expr;
```

This means `t` cannot be used as a variable name — it always evaluates to itself. This is the Common Lisp convention. Clojure avoids this by using `true` (a distinct type).

**Workaround**: Don't name variables `t`. Use `x`, `val`, `result`, etc. If porting Clojure code that uses `t` as a variable (rare), rename it.

### Adding `true`/`false` aliases

For Clojure compatibility, the prelude could add:
```lisp
(define true t)
(define false nil)
```

This provides readable boolean names while keeping the underlying implementation simple. `(if true ...)` and `(if false ...)` work as expected because `true` looks up to `t` and `false` looks up to `nil`.

## Patterns and Idioms

### Boolean logic

```lisp
(and (< 1 2) (< 2 3))    ;; => t (both true)
(or nil 42)                ;; => 42 (first truthy value — like Clojure)
(not nil)                  ;; => t
(not 42)                   ;; => nil
```

Note: `or` returns the truthy value itself, not `t`. This matches Clojure's behavior and enables the "or-as-default" pattern:
```lisp
(or (assoc 'key data) 'default)  ;; returns the found pair, or 'default
```

### Truthiness in practice

```lisp
;; Use nil as "not found"
(define result (assoc 'key data))
(if result (cdr result) 'missing)

;; Anaphoric version (cleaner)
(aif (assoc 'key data) (cdr it) 'missing)

;; Every value except nil is "something"
(define items (filter identity (list 1 nil 2 nil 3)))
;; => (1 2 3)
```

### Elisp "mistakes" we avoid

1. **No `eq` for numbers**: Elisp's `eq` compares identity, not value, for numbers. Our `=` compares fixnum values; `eq?` compares identity (same object).
2. **No dynamic scoping**: Elisp defaults to dynamic scope. We use lexical scope (closures work correctly).
3. **No confusion between `t` and non-nil**: We're consistent — `t` is the canonical true, but any non-nil works.

### What we could improve

1. **`true`/`false` prelude aliases**: Easy, adds readability for Clojure porters.
2. **`boolean?` predicate**: `(define boolean? (lambda (x) (or (eq? x t) (null? x))))`.
3. **`when-let`**: Clojure's `(when-let [x (find-thing)] (use x))` — our `awhen` already does this.
