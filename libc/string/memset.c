#include <string.h>

void *memset(void *s, int c, size_t n) {
    unsigned char *ptr = (unsigned char *)s;
    unsigned char val = (unsigned char)c;
    for (size_t i = 0; i < n; i++) {
        ptr[i] = val;
    }
    return s;
}
