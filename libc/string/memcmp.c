#include <string.h>

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
