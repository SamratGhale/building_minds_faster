void swap_bytes(void *pv, size_t n) {
    assert(n > 0);
    char *p =(char*)pv;
    size_t lo, hi;
    for(lo=0, hi=n-1; hi>lo; lo++, hi--) {
        char tmp=p[lo];
        p[lo] = p[hi];
        p[hi] = tmp;
    }
}
#define SWAP(x) swap_bytes(&x, sizeof(x));
