#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/io.h>

#include "gdt.hpp"
#include "heap.hpp"
#include "interrupt.hpp"
#include "multiboot.h"
#include "pmm.hpp"
#include "test/ktest.hpp"
#include "x86.hpp"

/* Verify we are using the i686-elf cross-compile */
#if !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

extern "C" void kernel_init() {
    assert(mboot_magic == MULTIBOOT_BOOTLOADER_MAGIC);
    GDT::init();
    Interrupt::init();
    Heap::init();
}

extern "C" __attribute__((noreturn)) void kernel_main() {
#ifdef KERNEL_TESTS
    KTest::run_all();  // runs all registered tests then exits QEMU
#else
    printf("Heap initialised: base=%p  size=%u KiB\n",
           reinterpret_cast<void *>(&kernel_end),
           static_cast<unsigned>(HEAP_SIZE / 1024));

    // --- Heap smoke test ---
    printf("Heap smoke test:\n");

    void *a = kmalloc(64);
    assert(a != nullptr);
    printf("  kmalloc(64)   -> %p\n", a);

    unsigned char *b = static_cast<unsigned char *>(kcalloc(4, 16));
    assert(b != nullptr);
    int all_zero = 1;
    for (int i = 0; i < 64; ++i)
        if (b[i] != 0) {
            all_zero = 0;
            break;
        }
    printf("  kcalloc(4,16) -> %p  zeroed=%s\n", static_cast<void *>(b),
           all_zero ? "yes" : "no");

    void *c = krealloc(a, 256);
    assert(c != nullptr);
    printf("  krealloc(a,256)-> %p\n", c);

    kfree(b);
    kfree(c);

    void *d = kmalloc(64);
    assert(d != nullptr);
    printf("  kmalloc after free -> %p\n", d);
    kfree(d);

    printf("Heap smoke test PASSED\n");
    Heap::dump();

    while (1) {
        asm volatile("hlt");
    }
#endif
    __builtin_unreachable();
}
