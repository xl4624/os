#include <stdio.h>

#if defined(__is_libk)

int getchar(void) {
    return EOF;
}

#elif defined(__is_libc)

    #include <unistd.h>

int getchar(void) {
    char c;
    int n = read(0, &c, 1);
    if (n <= 0) {
        return EOF;
    }
    return (unsigned char)c;
}

#endif
