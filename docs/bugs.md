# Known Bugs

## tc24r: Suspected stack frame issue with large local char arrays

**Status:** Worked around (buffer reduced from 128 to 32 bytes)

**Symptom:** `char buf[128]` in `read_string()` causes the while loop following it to not execute — the loop counter `i` remains 0 and `make_string` receives `len=0`. Changing to `char buf[32]` fixes the issue.

**Reproduction:** Only occurs within the tml24c call context (deep call chain: repl → read_str → read_expr → read_list → read_expr → read_string). A standalone tc24r test with the same `char buf[128]` pattern works correctly. The bug may be related to specific stack depth, frame layout, or interaction with GC-protected reader code.

**Workaround:** Keep local char arrays in `read_string` at 32 bytes, limiting string literals to 31 characters.

**Note:** Not reproducible in isolation. Root cause unknown — may be a tc24r code generation edge case at specific call depths.
