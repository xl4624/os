#include <string.h>

char *strcat(char *__restrict__ dest, const char *__restrict__ src) {
    char *ptr = dest;

    while (*ptr != '\0') {
        ++ptr;
    }

    while (*src != '\0') {
        *ptr++ = *src++;
    }
    *ptr = '\0';

    return dest;
}
