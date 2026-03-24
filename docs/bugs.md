# Known Bugs

## tc24r: Large local char arrays corrupt stack (Bug #1)

**Status:** Worked around in tml24c (reduced buffer from 128 to 32 bytes)

**Symptom:** `char buf[128]` in a function causes the while loop following it to not execute. The `sub sp, 131` instruction appears to not allocate the correct stack space, causing local variable `i` (stored at the end of the frame) to read as 0 regardless of writes.

**Reproduction:** In `read_string()`, with `char buf[128]; int i = 0;` and `sub sp, 131`, the while loop that increments `i` and fills `buf` doesn't execute — `i` remains 0. Changing to `char buf[32]` (generating `sub sp, 35`) fixes the issue completely.

**Root cause:** Likely a tc24r compiler bug with large stack frames. The `sub sp, N` instruction may not handle N > ~64 correctly, or the local variable offset calculation is wrong for frames exceeding a certain size.

**Workaround:** Keep local char arrays under 64 bytes. For string parsing, `char buf[32]` limits string literals to 31 characters, which is acceptable for the current use case.

**Affected project:** tc24r (Tiny COR24 compiler)
**Filed:** Not yet (needs tc24r issue tracker)
