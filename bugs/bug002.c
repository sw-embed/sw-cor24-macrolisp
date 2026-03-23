// Bug 002: Compiler panic on two-level #define constant expression
// tc24r panics in tc24r-emit-load-store when TAG_SYMBOL is used inside NIL_VAL

#define TAG_SYMBOL 2
#define NIL_VAL ((0 << 2) | TAG_SYMBOL)

int arr[10];

int main() {
    arr[0] = NIL_VAL;
    return arr[0];
}
