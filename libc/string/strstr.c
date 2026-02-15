#include <string.h>

char *strstr(const char *haystack, const char *needle) {
    if (*needle == '\0') {
        return (char *)haystack;
    }

    for (const char *h = haystack; *h != '\0'; h++) {
        if (*h == *needle) {
            const char *h2 = h;
            const char *n2 = needle;

            while (*n2 != '\0' && *h2 == *n2) {
                h2++;
                n2++;
            }

            if (*n2 == '\0') {
                return (char *)h;
            }
        }
    }

    return NULL;
}
