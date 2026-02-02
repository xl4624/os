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

// Hand-encoded user-mode test program (user/hello.S).
// TODO: Replace with a proper build step that assembles user programs
// separately and embeds the flat binary via objcopy.
#include "../../user/hello.h"

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
    Scheduler::create_process(kUserProgram, sizeof(kUserProgram),
                              reinterpret_cast<const uint8_t *>(kUserMessage),
                              kUserMessageLen);
    Scheduler::create_process(kUserProgram, sizeof(kUserProgram),
                              reinterpret_cast<const uint8_t *>(kUserMessage),
                              kUserMessageLen);
    printf("Starting scheduler...\n");
    while (true) {
        asm volatile("hlt");
    }
#endif
    __builtin_unreachable();
}

__END_DECLS
