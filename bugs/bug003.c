// Bug 003: Nested array indexing fails to parse
// pool[offsets[i]] causes "expected Semicolon, got LBracket"

char pool[100];
int offsets[10];

char *get(int i) {
    return &pool[offsets[i]];
}

int main() { return 0; }
