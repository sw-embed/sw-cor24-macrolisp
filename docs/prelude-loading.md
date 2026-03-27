# Prelude Loading Architecture

## Two Approaches

### 1. Interpreted (default): eval_str at startup

Each prelude variant is a C header with `eval_str()` calls:

```c
void load_prelude() {
    eval_str("(define (map f lst) ...)");
    eval_str("(define (filter p lst) ...)");
    // ... 60-90 definitions
}
```

**Cost:** ~7M COR24 instructions for standard prelude.

**Pros:** Simple, self-contained, no extra build steps.
**Cons:** Repeats read+eval work every startup.

### 2. Snapshot (10x faster): heap state restored from binary

Prelude state (heap, symbol table, string pool) is serialized to a binary
snapshot and restored via `--load-binary`:

```bash
# One-time: generate snapshot
just snapshot

# Fast startup: restore from snapshot
just run-fast
# or: cor24-run --run build/repl-snapshot.s --load-binary build/prelude.snap@0x080000 --terminal --echo --speed 0
```

**Cost:** ~700K instructions (memcpy of ~12KB snapshot).

**Pros:** 10x faster startup. Same prelude, no functional difference.
**Cons:** Extra build step (`just snapshot`). Snapshot must be regenerated
when prelude definitions change. Falls back to eval_str if no snapshot.

## How Snapshots Work

### Save (just snapshot)

1. `snapshot-save.c` loads the standard prelude normally
2. Forces a GC to compact the heap
3. Serializes all interpreter state to UART as hex:
   - Scalar state: heap_next, free_list, global_env, sym_count, etc.
   - Pre-interned symbols: sym_quote, sym_if, sym_define, ...
   - Heap arrays: heap_car[0..heap_next], heap_cdr[0..heap_next]
   - Symbol table: name_pool, sym_name_off
   - String pool: str_pool
4. `scripts/extract-snapshot.py` converts hex to binary `build/prelude.snap`

### Restore (repl-snapshot.c)

1. Checks for TML magic at `SNAPSHOT_ADDR` (0x080000)
2. If found: reads binary snapshot, restores all arrays and variables
3. If not found: falls back to `load_prelude()` (eval_str path)

### Snapshot Format

All integers are 24-bit little-endian. After 3-byte "TML" magic:

| Field | Size | Description |
|-------|------|-------------|
| heap_next | 3 | Number of used heap cells |
| free_list | 3 | Free list head |
| global_env | 3 | Environment root |
| sym_count | 3 | Number of symbols |
| name_pool_next | 3 | Symbol name pool fill |
| str_pool_next | 3 | String pool fill |
| gensym_counter | 3 | Gensym state |
| pre-interned syms | 33 | 11 special form symbols |
| heap_car | 3*heap_next | Heap car array |
| heap_cdr | 3*heap_next | Heap cdr array |
| name_pool | name_pool_next | Symbol name bytes |
| sym_name_off | 3*sym_count | Symbol name offsets |
| str_pool | str_pool_next | String data bytes |

Typical size: ~12KB for standard prelude (~1800 heap cells).

## Performance Comparison

| Method | Instructions to REPL | Speedup |
|--------|---------------------|---------|
| eval_str (standard) | ~7,000,000 | 1x (baseline) |
| eval_str (full) | ~15,000,000 | 0.5x |
| Snapshot restore | ~700,000 | **10x** |
| Bare (no prelude) | ~100,000 | 70x |

## Justfile Recipes

| Recipe | Description |
|--------|-------------|
| `just run` | Standard REPL (eval_str prelude) |
| `just run-fast` | Snapshot REPL (10x faster startup) |
| `just snapshot` | Generate prelude snapshot |
| `just build-snapshot` | Build snapshot REPL binary |
| `just build-snapshot-save` | Build snapshot generator |

## Future: cor24-rs Binary Loader

The cor24-rs emulator's `--load-binary` flag already supports loading raw
bytes at arbitrary addresses. A future enhancement could:

1. Pre-assemble the REPL binary (skip assembly step)
2. Embed the snapshot directly in the binary's data segment
3. Eliminate the external snapshot file entirely
