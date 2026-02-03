#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/io.h>

#include "gdt.h"
#include "heap.h"
#include "interrupt.h"
#include "keyboard.h"
#include "multiboot.h"
#include "paging.h"
#include "pit.h"
#include "pmm.h"
#include "scheduler.h"
#include "syscall.h"
#include "test/ktest.h"
#include "tss.h"
#include "x86.h"

/* Verify we are using the i686-elf cross-compile */
#if !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

__BEGIN_DECLS

// Kernel stack symbol defined in arch/boot.S
extern char stack_top[];

void kernel_init() {
    assert(mboot_magic == MULTIBOOT_BOOTLOADER_MAGIC);
    GDT::init();
    TSS::init();
    TSS::set_kernel_stack(reinterpret_cast<uint32_t>(stack_top));
    Interrupt::init();
    Syscall::init();
    PIT::init();
    kHeap.init();
    Scheduler::init();
}

__attribute__((noreturn)) void kernel_main() {
#ifdef KERNEL_TESTS
    KTest::run_all();  // runs all registered tests then exits QEMU
#else
    Scheduler::start();

    // Load user programs from multiboot modules.
    const auto *info = reinterpret_cast<const multiboot_info_t *>(
        phys_to_virt(reinterpret_cast<paddr_t>(mboot_info)));
    if ((info->flags & MULTIBOOT_INFO_MODS) && info->mods_count > 0) {
        const auto *mods = reinterpret_cast<const multiboot_module_t *>(
            phys_to_virt(info->mods_addr));
        for (uint32_t i = 0; i < info->mods_count; ++i) {
            const auto *data = reinterpret_cast<const uint8_t *>(
                phys_to_virt(mods[i].mod_start));
            const size_t len = mods[i].mod_end - mods[i].mod_start;
            printf("Loading module %u (%u bytes)...\n", i,
                   static_cast<unsigned>(len));
            Scheduler::create_process(data, len);
        }
    } else {
        printf("No multiboot modules found.\n");
    }

    printf("Starting scheduler...\n");
    while (true) {
        asm volatile("hlt");
    }
#endif
    __builtin_unreachable();
}

__END_DECLS
