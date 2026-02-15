#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gdt.hpp"
#include "heap.hpp"
#include "interrupt.hpp"
#include "multiboot.h"
#include "pmm.hpp"
#include "x86.hpp"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__) || !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

extern "C" void kernel_init() {
    assert(mboot_magic == MULTIBOOT_BOOTLOADER_MAGIC);
    GDT::init();
    Interrupt::init();
    Heap::init();
}

extern "C" __attribute__((noreturn)) void kernel_main() {
    printf("Heap initialised: base=%p  size=%u KiB\n",
           reinterpret_cast<void *>(&kernel_end),
           static_cast<unsigned>(HEAP_SIZE / 1024));

    // --- Heap smoke test ---
    printf("Heap smoke test:\n");

    // Basic alloc/free
    void *a = kmalloc(64);
    assert(a != nullptr);
    printf("  kmalloc(64)   -> %p\n", a);

    // calloc must return zeroed memory
    unsigned char *b = static_cast<unsigned char *>(kcalloc(4, 16));
    assert(b != nullptr);
    int all_zero = 1;
    for (int i = 0; i < 64; ++i) if (b[i] != 0) { all_zero = 0; break; }
    printf("  kcalloc(4,16) -> %p  zeroed=%s\n", static_cast<void *>(b),
           all_zero ? "yes" : "no");

    // realloc grow: data should be preserved
    void *c = krealloc(a, 256);
    assert(c != nullptr);
    printf("  krealloc(a,256)-> %p\n", c);

    // Free all allocations
    kfree(b);
    kfree(c);

    // Allocate again — should reuse the freed space
    void *d = kmalloc(64);
    assert(d != nullptr);
    printf("  kmalloc after free -> %p\n", d);
    kfree(d);

    printf("Heap smoke test PASSED\n");
    Heap::dump();

    while (1) {
        asm volatile("hlt");
    }
    __builtin_unreachable();
}
