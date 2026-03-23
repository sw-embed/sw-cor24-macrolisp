// Bug 001: Nested function-like macro not expanded
// tc24r treats MAKE_SYMBOL(0) as a function call instead of expanding it

#define MAKE_SYMBOL(idx) (((idx) << 2) | 2)
#define NIL_VAL MAKE_SYMBOL(0)

int main() {
    int x = NIL_VAL;
    return x;
}
