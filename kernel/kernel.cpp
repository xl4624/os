#include <stdio.h>
#include <string.h>

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__) || !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

int test_divide_by_zero() {
    volatile int a = 10;
    volatile int b = 0;
    volatile int c = a / b;  // This should trigger isr0_handler
    return c;
}

extern "C" void kmain(void) {
    printf("Hello world!\n");
    const char *test1 = "testing";
    const char *test2 = "testimg";
    int r = memcmp((void *)test1, (void *)test2, 7);
    printf("memcmp test: %d\n", r);
    printf("testing: %s\n", test2);
    printf("testing: %d\n", 32);
    while (1) {
        __asm__ volatile("hlt");
    }
}
