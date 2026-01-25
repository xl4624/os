#include <string.h>

size_t strspn(const char *s, const char *accept) {
    size_t count = 0;

    for (const char *p = s; *p != '\0'; ++p) {
        int found = 0;
        for (const char *a = accept; *a != '\0'; ++a) {
            if (*p == *a) {
                found = 1;
                break;
            }
        }
        if (!found) {
            break;
        }
        count++;
    }

    return count;
}
