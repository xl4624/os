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
#include "vmm.hpp"
#include "x86.hpp"

/* Verify we are using the i686-elf cross-compile */
#if !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

extern "C" void kernel_init() {
    assert(mboot_magic == MULTIBOOT_BOOTLOADER_MAGIC);
    GDT::init();
    Interrupt::init();
    kHeap.init();
}

extern "C" __attribute__((noreturn)) void kernel_main() {
#ifdef KERNEL_TESTS
    KTest::run_all();  // runs all registered tests then exits QEMU
#else
    printf("Heap initialised: base=%p  initial=%u KiB  max=%u MiB\n",
           reinterpret_cast<void *>(Heap::kVirtBase),
           static_cast<unsigned>(Heap::kInitialPages * 4),
           static_cast<unsigned>(Heap::kMaxSize / (1024 * 1024)));

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
    kHeap.dump();

    printf("\nVMM smoke test:\n");
    {
        // Map a fresh physical page into the kernel VA space just past the
        // boot-mapped 8 MiB window and write through it.
        constexpr vaddr_t VA = 0xC0800000;
        const paddr_t phys = kPmm.alloc();
        assert(phys != 0);
        VMM::map(VA, phys);

        volatile uint32_t *p = reinterpret_cast<volatile uint32_t *>(VA);
        *p = 0xCAFEBABE;

        printf("  mapped virt=%p -> phys=%p\n", reinterpret_cast<void *>(VA),
               reinterpret_cast<void *>(VMM::get_phys(VA)));
        printf("  wrote 0xcafebabe, read back 0x%08x\n",
               static_cast<unsigned>(*p));
        assert(*p == 0xCAFEBABE);

        VMM::unmap(VA);
        assert(VMM::get_phys(VA) == 0);
        kPmm.free(phys);
        printf("  unmapped and freed -- OK\n");
    }
    printf("VMM smoke test PASSED\n");

    while (true) {
        asm volatile("hlt");
    }
#endif
    __builtin_unreachable();
}
