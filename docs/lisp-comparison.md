# How tml24c Compares to Other Lisps

## At a Glance

| Feature | tml24c | Clojure | Common Lisp | Emacs Lisp | Tiny Lisps |
|---------|--------|---------|-------------|------------|------------|
| **Target** | COR24 bare metal | JVM/JS | Native/bytecode | Emacs VM | Various embedded |
| **Word size** | 24-bit | 64-bit | 32/64-bit | 64-bit | 16-32 bit |
| **Scope** | Lexical | Lexical | Lexical (default) | Dynamic (default) | Varies |
| **Namespace** | Lisp-1 | Lisp-1 | Lisp-2 | Lisp-2 | Usually Lisp-1 |
| **GC** | Conservative mark-sweep | Generational | Generational | Mark-sweep | Simple/none |
| **Macros** | Unhygienic defmacro | Unhygienic (by convention) | Unhygienic | Unhygienic | Varies |
| **TCO** | Yes (if, begin, apply) | recur only | Implementation-dependent | No | Some |
| **Strings** | Immutable, 119 char | Immutable, Java String | Mutable | Mutable | Often absent |
| **Closures** | Yes | Yes | Yes | Since Emacs 24 | Some |
| **REPL** | Yes (UART) | Yes (nREPL) | Yes | Yes (*scratch*) | Some |

## Detailed Comparison

### vs. Clojure / ClojureScript

tml24c is most influenced by Clojure's philosophy. Shared traits:
- **Lisp-1**: functions and values share one namespace
- **Nil punning**: `nil` is both false and empty
- **Functional prelude**: `map`, `filter`, `reduce`, `partial`, `complement`, `juxt`
- **Unhygienic macros** with quasiquote
- **Immutable strings**
- **`fn?`** predicate for function detection
- **Trampolining** for mutual recursion

Key differences:
| | tml24c | Clojure |
|-|--------|---------|
| Persistent data structures | No (cons lists only) | Yes (vectors, maps, sets via HAMTs) |
| Lazy sequences | Basic (thunk-in-cdr) | Core feature (lazy-seq everywhere) |
| Keywords | No (use symbols) | Yes (`:foo`) |
| Destructuring | No | Yes (in let, fn, loop) |
| Multimethods | Future (alist-based) | Built-in (defmulti/defmethod) |
| Protocols | No | Yes |
| Atoms/refs/agents | No (set! only) | Yes (STM, atoms, agents) |
| Spec/schema | No | clojure.spec |
| Threading macros | Two-arg only | Variadic + many variants |
| `loop`/`recur` | TCO on all tail calls | Explicit `recur` only |
| Namespaces | Single global | Rich namespace system |
| Java/JS interop | N/A | Core feature |
| Platform | 24-bit bare metal | JVM or browser |

ClojureScript is closer in spirit (compiles to a constrained target) but still has the full Clojure data structure library.

### vs. Common Lisp

tml24c shares CL's traditional Lisp roots:
- **nil = false = empty list** (same convention)
- **`t`** as canonical true
- **`defmacro`** with unhygienic expansion
- **`car`/`cdr`** naming
- **`cond`** with `t` default

Key differences:
| | tml24c | Common Lisp |
|-|--------|-------------|
| Namespace | Lisp-1 | Lisp-2 (separate function/value) |
| `defun` | `(define f (lambda ...))` | `(defun f (args) body)` |
| `funcall`/`function` | Not needed (Lisp-1) | Required for higher-order |
| CLOS (OOP) | No | Yes (generic functions, classes) |
| Conditions/restarts | No | Yes (powerful error handling) |
| Format strings | No (`display` only) | Yes (`format`) |
| Multiple values | No | Yes (`values`, `multiple-value-bind`) |
| Packages | No | Yes |
| `setf` | `set!` (simpler) | `setf` (generalized place mutation) |
| Tail calls | Yes (guaranteed) | Implementation-dependent |
| Standard | No | ANSI CL |
| Implementations | 1 (COR24) | Many (SBCL, CCL, ECL, ABCL...) |

### vs. Emacs Lisp

tml24c avoids several Elisp design decisions:
- **Lexical scope** (Elisp defaulted to dynamic scope until Emacs 24)
- **Lisp-1** (Elisp is Lisp-2 like CL)
- **Closures always work** (Elisp closures require `lexical-binding: t`)
- **No advice/hooks** — tml24c is simpler

Shared traits:
- `nil` = false = empty list
- `t` as true
- Unhygienic macros
- Single-threaded

Key differences:
| | tml24c | Emacs Lisp |
|-|--------|------------|
| Scope | Lexical | Dynamic (default), lexical (opt-in) |
| Namespace | Lisp-1 | Lisp-2 |
| Purpose | Bare metal | Text editor extension |
| Buffer/text | No | Core feature |
| `defvar`/`defconst` | No | Yes (dynamic variables) |
| Advice system | No | Yes (`advice-add`) |
| Byte compilation | No | Yes |

### vs. Tiny Lisps

Several projects target constrained environments like tml24c:

| Project | Target | Word size | GC | Strings | Macros | Closures |
|---------|--------|-----------|-----|---------|--------|----------|
| **tml24c** | COR24 (24-bit) | 24 | Conservative mark-sweep | Yes | defmacro + quasiquote | Yes |
| **uLisp** | Arduino/ARM | 32 | Mark-sweep | Some | No | No |
| **femtolisp** | Desktop | 64 | Copying | Yes | Yes | Yes |
| **picolisp** | Desktop/RPi | 64 | Mark-sweep | Yes | No (uses `de`) | No (uses dynamic) |
| **TinyScheme** | Embedded | 32 | Mark-sweep | Yes | Hygienic (R5RS) | Yes |
| **s7** | Desktop | 64 | Mark-sweep | Yes | defmacro | Yes |
| **MAL** (Make a Lisp) | Various | Host | Host | Yes | defmacro | Yes |

**tml24c's niche**: the only Lisp targeting a 24-bit architecture with full closures, quasiquote macros, conservative GC, TCO, and a Clojure-influenced prelude.

### vs. Scheme family (Guile, Racket, Chicken, R7RS)

| Aspect | Schemes | tml24c |
|--------|---------|--------|
| **Hygiene** | Hygienic macros (`syntax-rules`, `syntax-case`) | Unhygienic `defmacro` |
| **Naming** | `define`, `lambda`, `call-with-current-continuation` | Same core, shorter names |
| **Boolean** | `#t`/`#f` (distinct type) | `t`/`nil` (symbols) |
| **Empty list** | `'()` (not `#f`) | `nil` = `'()` = false |
| **Tail calls** | Guaranteed (R5RS+) | Guaranteed |
| **Continuations** | `call/cc` | No |
| **Modules** | R7RS libraries | No |
| **Standard** | R5RS, R7RS | No formal standard |

The main philosophical split: Schemes prefer **hygiene** (macros can't accidentally capture variables). tml24c embraces **unhygienic macros** — `aif`/`awhen` intentionally capture `it`, which is a feature, not a bug. This aligns with the CL/Clojure tradition over the Scheme tradition.

## Design Philosophy

tml24c's design choices:

1. **Clojure-influenced, CL-rooted**: Functional prelude from Clojure, nil/t conventions from CL
2. **Unhygienic by choice**: Enables anaphoric macros and simpler implementation
3. **Lexical scope, Lisp-1**: Modern consensus, no `funcall` needed
4. **Conservative GC**: Correctness over precision — never collects live objects
5. **Display over str**: Direct I/O for performance on constrained hardware
6. **Single-line primitives, multi-line REPL**: Simple reader with paren-depth tracking
7. **No formal standard**: Pragmatic additions as needed for the target platform
