#include <termctl.h>

#if defined(__is_libk)

void set_cursor(unsigned int row, unsigned int col) {
    (void)row;
    (void)col;
}

void set_color(unsigned char color) {
    (void)color;
}

void clear_screen(void) {
}

#elif defined(__is_libc)

    #include <sys/syscall.h>

void set_cursor(unsigned int row, unsigned int col) {
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SET_CURSOR), "b"(row), "c"(col));
    (void)ret;
}

void set_color(unsigned char color) {
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_SET_COLOR), "b"((unsigned int)color));
    (void)ret;
}

void clear_screen(void) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_CLEAR));
    (void)ret;
}

#endif
