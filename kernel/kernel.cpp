#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gdt.hpp"
#include "interrupt.hpp"
#include "x86.hpp"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__) || !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

multiboot_info_t *multiboot_data;

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

extern "C" __attribute__((noreturn)) void kernel_main(uint32_t magic,
                                                      multiboot_info_t *mbd) {
    assert(magic == MULTIBOOT_BOOTLOADER_MAGIC);
    multiboot_data = mbd;

    printf("Hello world!\n");
    char test1[] = "testing";
    char test2[] = "samples";
    assert(memcmp((void *)test1, (void *)test2, 8) == 1);

    void *r = memmove((void *)test1, (void *)test2, 8);
    // should return test2's contents in test1's memory address
    assert(memcmp(r, test2, 8) == 0);
    assert(r == test1);

    printf("kernel_start: %p\n", &kernel_start);
    printf("kernel_end: %p\n", &kernel_end);

    while (1) {
        asm volatile("hlt");
    }
    __builtin_unreachable();
}
