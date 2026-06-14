# Known Bugs and Limitations

## Fixed

### Dotted-pair reader bug (fixed in step 4)
`read_list` wasn't consuming `)` for first-element dotted pairs. `(lambda (a . rs) body)` was parsed as `(lambda (a . rs))` — body silently dropped. Root cause of "variadic params don't work" reports.

### GC collecting live objects (fixed: conservative stack scanning)
Mark-sweep GC now scans the COR24 EBR stack for tagged heap pointers. Also made `gc_mark_val` iterative on cdr to prevent stack overflow during marking of large global_env.

### REPL single-line limit (fixed: paren-depth tracking)
`read_line` now tracks `(` / `)` depth and continues reading across newlines. Buffer is 1024 bytes.

### String literal limit (fixed: raised to 119 chars)
Original 32-byte buffer was a workaround for the dotted-pair reader bug, not a tc24r compiler issue.

## Open

### Symbol `t` cannot be used as a variable name
`eval` checks `expr == T_VAL` before environment lookup, so `t` always evaluates to itself regardless of bindings. Common CL behavior but a trap for new users. Workaround: don't name variables `t`.

### String pool is append-only
`str_pool` (2048 bytes) grows monotonically. String data is never reclaimed even when the heap cell is GC'd. Long-running programs that create many strings will exhaust the pool. Fix: either compact the pool during GC or allocate strings in heap cells.

### `or` macro evaluates first argument twice
`(or a b)` expands to `(if a a b)` — `a` is evaluated twice. If `a` has side effects, this is wrong. Fix: needs `gensym` or expand to `(let ((tmp a)) (if tmp tmp b))`.

### `and`/`or` are two-argument only
Clojure's `and`/`or` are variadic. Ours only take two args. Nest for more: `(and a (and b c))`.

## Limitations (by design)

| Limit | Value | Notes |
|-------|-------|-------|
| Heap cells | 4096 | Mark-sweep GC reclaims unused cells |
| String pool | 2048 bytes | Append-only, not reclaimed |
| String literal | 119 characters | Reader buffer limit |
| REPL line | 1024 characters | With paren-depth continuation |
| GC root stack | 256 entries | Mostly unused with conservative GC |
| Symbol table | 256 symbols / 2048 bytes | Interned, never freed |
| Stack (EBR) | 3–8 KB | `--stack-kilobytes` flag in cor24-emu |
| Fixnum range | -2,097,152 to 2,097,151 | 22-bit signed (24-bit word, 2-bit tag) |
