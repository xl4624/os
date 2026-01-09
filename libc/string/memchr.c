#include <string.h>

void *memchr(const void *s, int c, size_t n) {
    const unsigned char *ptr = (const unsigned char *)s;
    unsigned char ch = (unsigned char)c;

    for (size_t i = 0; i < n; i++) {
        if (ptr[i] == ch) {
            return (void *)(ptr + i);
        }
    }
    return NULL;
}
