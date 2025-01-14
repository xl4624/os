#include <string.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    unsigned char *destination = (unsigned char *)dest;
    const unsigned char *source = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) {
        destination[i] = source[i];
    }
    return dest;
}
