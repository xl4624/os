#include <string.h>

size_t strcspn(const char *s, const char *reject) {
    size_t count = 0;

    for (const char *p = s; *p != '\0'; ++p) {
        for (const char *r = reject; *r != '\0'; ++r) {
            if (*p == *r) {
                return count;
            }
        }
        ++count;
    }

    return count;
}
