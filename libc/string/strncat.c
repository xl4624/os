#include <string.h>

char *strncat(char *__restrict__ dest, const char *__restrict__ src, size_t n) {
    char *ptr = dest;

    while (*ptr != '\0') {
        ++ptr;
    }

    while (n > 0 && *src != '\0') {
        *ptr++ = *src++;
        --n;
    }
    *ptr = '\0';

    return dest;
}
