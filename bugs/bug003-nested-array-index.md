# Bug 003: Nested array indexing fails to parse

## Summary

Using one array index expression as the subscript for another array causes a parse error: "expected Semicolon, got LBracket".

## Minimal reproducer

```c
// bug003.c
char pool[100];
int offsets[10];

char *get(int i) {
    return &pool[offsets[i]];
}

int main() { return 0; }
```

## Command

```
tc24r bug003.c -o bug003.s
```

## Actual output

```
tc24r: error at offset 68: expected Semicolon, got LBracket
```

## Expected output

Should compile successfully. `pool[offsets[i]]` is valid C — the inner `offsets[i]` evaluates to an int, which is used as the subscript for `pool`.

## Workaround

Use a temporary variable:

```c
char *get(int i) {
    int off = offsets[i];
    return &pool[off];
}
```
