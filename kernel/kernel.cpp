#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/io.h>

#include "../tests/kernel/ktest.h"
#include "gdt.h"
#include "heap.h"
#include "interrupt.h"
#include "keyboard.h"
#include "modules.h"
#include "multiboot.h"
#include "paging.h"
#include "pit.h"
#include "pmm.h"
#include "scheduler.h"
#include "syscall.h"
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
  const auto* info = phys_to_virt(paddr_t{mboot_info}).ptr<multiboot_info_t>();
  Modules::init(info);

  if (Modules::count() > 0) {
    // Launch module 0 (the shell); other modules stay in the registry
    // for the shell to exec by name.
    const Module* shell = Modules::get(0);
    printf("Loading shell \"%s\" (%u bytes)...\n", shell->name, static_cast<unsigned>(shell->len));
    Scheduler::create_process(shell->data, shell->len, shell->name);
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
