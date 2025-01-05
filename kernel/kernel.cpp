#include <stdio.h>
#include <string.h>

#include "gdt.hpp"
#include "interrupt.hpp"

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

extern "C" void kernel_init() {
    GDT::init();
    interrupt_init();
}

extern "C" void kernel_main() {
    printf("Hello world!\n");
    char test1[] = "testing";
    char test2[] = "testimg";
    int r1 = memcmp((void *)test1, (void *)test2, 7);
    void *r2 = memmove((void *)test1, (void *)test2, 8);

    printf("memcmp test: %d\n", r1);  // should return 1

    // should return test2's contents in test1's memory address
    printf("memmove test: %s\n", r2);
    printf("memmove test: %d\n", r2 == test1);

    printf("testing: %s\n", "testing");
    printf("testing: %d\n", 32);

    while (1) {
        asm("hlt");
    }
}
