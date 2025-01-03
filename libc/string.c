#include "string.h"

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

// The way this works is by casting to unsigned char and comparing
int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *first = (const unsigned char *)s1;
    const unsigned char *second = (const unsigned char *)s2;
    for (size_t i = 0; i < n; i++) {
        if (first[i] < second[i])
            return -1;
        else if (first[i] > second[i])
            return 1;
    }
    return 0;
}

void *memcpy(void *__restrict dest, const void *__restrict src, size_t n) {
    unsigned char *destination = (unsigned char *)dest;
    const unsigned char *source = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) {
        destination[i] = source[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    // Choose direction by checking where src and dest are relative to each
    // other to avoid overwriting
    if (d > s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i-- > 0;) {
            d[i] = s[i];
        }
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *ptr = (unsigned char *)s;
    unsigned char val = (unsigned char)c;
    for (size_t i = 0; i < n; i++) {
        ptr[i] = val;
    }
    return s;
}
