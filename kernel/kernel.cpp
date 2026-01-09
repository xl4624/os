#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gdt.hpp"
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
    interrupt_init();
}

extern "C" __attribute__((noreturn)) void kernel_main() {
    printf("=== myos PMM demo ===\n\n");

    // --- Memory layout ---
    printf("kernel_start : %p\n", &kernel_start);
    printf("kernel_end   : %p\n", &kernel_end);
    printf("\n");

    // --- PMM stats after init ---
    printf("PMM initialised\n");
    printf("  total frames : %zu\n",
           pmm.get_free_count() + pmm.get_used_count());
    printf("  free  frames : %zu  (%zu KiB)\n", pmm.get_free_count(),
           pmm.get_free_count() * 4);
    printf("  used  frames : %zu  (%zu KiB)\n", pmm.get_used_count(),
           pmm.get_used_count() * 4);
    printf("\n");

    // --- Bitmap snapshot: first 256 frames (first 1 MiB), 1=used 0=free ---
    printf("Bitmap first 256 frames (1=used 0=free):\n");
    pmm.print_range(0, 256);
    printf("\n");

    // --- Allocate a few pages and show their addresses ---
    printf("Allocating 4 pages:\n");
    paddr_t pages[4];
    for (int i = 0; i < 4; ++i) {
        pages[i] = pmm.alloc();
        printf("  page[%d] = %p\n", i, reinterpret_cast<void *>(pages[i]));
    }
    printf("  free frames after alloc: %zu\n", pmm.get_free_count());
    printf("\n");

    // --- Free the middle two and re-check the count ---
    printf("Freeing page[1] and page[2]:\n");
    pmm.free(pages[1]);
    pmm.free(pages[2]);
    printf("  free frames after free : %zu\n", pmm.get_free_count());
    printf("\n");

    // --- Verify the freed pages are re-issued on the next alloc ---
    printf("Re-allocating 2 pages (should reuse freed frames):\n");
    paddr_t reused0 = pmm.alloc();
    paddr_t reused1 = pmm.alloc();
    printf("  reused[0] = %p  (was page[1] = %p) match=%s\n",
           reinterpret_cast<void *>(reused0),
           reinterpret_cast<void *>(pages[1]),
           reused0 == pages[1] ? "yes" : "no");
    printf("  reused[1] = %p  (was page[2] = %p) match=%s\n",
           reinterpret_cast<void *>(reused1),
           reinterpret_cast<void *>(pages[2]),
           reused1 == pages[2] ? "yes" : "no");

    while (1) {
        asm volatile("hlt");
    }
    __builtin_unreachable();
}
