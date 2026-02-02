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
#include "syscall.h"
#include "test/ktest.h"
#include "tss.h"
#include "usermode.h"
#include "vmm.h"
#include "x86.h"

// Hand-encoded user-mode test program (user/hello.S).
// TODO: Replace with a proper build step that assembles user programs
// separately and embeds the flat binary via objcopy.
#include "../../user/hello.h"

// Kernel stack symbol defined in arch/boot.S
extern "C" char stack_top[];

/* Verify we are using the i686-elf cross-compile */
#if !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

extern "C" void kernel_init() {
    assert(mboot_magic == MULTIBOOT_BOOTLOADER_MAGIC);
    GDT::init();
    TSS::init();
    TSS::set_kernel_stack(reinterpret_cast<uint32_t>(stack_top));
    Interrupt::init();
    Syscall::init();
    PIT::init();
    kHeap.init();
}

// TODO: Replace this ad-hoc loader with an ELF loader once we have a filesystem
// and proper process creation (Milestone 4+).
static void load_user_program() {
    // --- Map user code page ---
    paddr_t code_phys = kPmm.alloc();
    assert(code_phys != 0);
    VMM::map(USER_CODE_VA, code_phys, /*writeable=*/true, /*user=*/true);

    // Copy the user program (instructions + message) into the user page.
    auto *dest = reinterpret_cast<uint8_t *>(USER_CODE_VA);
    memcpy(dest, kUserProgram, sizeof(kUserProgram));
    memcpy(dest + kUserCodeLen, kUserMessage, kUserMessageLen);

    printf("  user code at %p (phys %p), %u bytes\n",
           reinterpret_cast<void *>(USER_CODE_VA),
           reinterpret_cast<void *>(code_phys),
           static_cast<unsigned>(sizeof(kUserProgram) + kUserMessageLen));

    // --- Map user stack pages ---
    // TODO: Stack guard page (unmap page below stack to catch overflow).
    for (uint32_t i = 0; i < USER_STACK_PAGES; ++i) {
        paddr_t stack_phys = kPmm.alloc();
        assert(stack_phys != 0);
        VMM::map(USER_STACK_VA + i * PAGE_SIZE, stack_phys,
                 /*writeable=*/true, /*user=*/true);
    }

    printf("  user stack at %p-%p (%u pages)\n",
           reinterpret_cast<void *>(USER_STACK_VA),
           reinterpret_cast<void *>(USER_STACK_TOP), USER_STACK_PAGES);
}

extern "C" __attribute__((noreturn)) void kernel_main() {
#ifdef KERNEL_TESTS
    KTest::run_all();  // runs all registered tests then exits QEMU
#else
    printf("Entering usermode test...\n");

    load_user_program();

    printf("  jumping to ring 3...\n");
    Usermode::jump(USER_CODE_VA, USER_STACK_TOP);
#endif
    __builtin_unreachable();
}
