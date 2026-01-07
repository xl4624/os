#include <string.h>

char *strchr(const char *s, int c) {
    const char ch = (char)c;

    // iterate until we find c or hit end of string
    while (*s != '\0' && *s != ch)
        s++;

    // return pointer to ch if found, NULL if not
    return *s == ch ? (char *)s : NULL;
}

char *strrchr(const char *s, int c) {
    const char ch = (char)c;

    // if we are searching for '\0', just return a pointer to end of string
    if (ch == '\0') {
        while (*s)
            s++;
        return (char *)s;
    }

    char *last = NULL;
    for (; *s; s++) {
        if (*s == ch) {
            last = (char *)s;
        }
    }
    return last;
}
