#include <assert.h>
#include <span.h>
#include <stdio.h>
#include <sys/io.h>

#include "../tests/kernel/ktest.h"
#include "framebuffer.h"
#include "gdt.h"
#include "heap.h"
#include "interrupt.h"
#include "modules.h"
#include "multiboot.h"
#include "paging.h"
#include "pit.h"
#include "scheduler.h"
#include "syscall.h"
#include "terminal.h"
#include "tss.h"
#include "vfs.h"
#include "x86.h"

/* Verify we are using the i686-elf cross-compile */
#ifndef __i386__
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
  {
    const auto* ktest_info = phys_to_virt(paddr_t{mboot_info}).ptr<multiboot_info_t>();
    if (Framebuffer::init(ktest_info)) {
      kTerminal.init();
    }
  }
  KTest::run_all();  // runs all registered tests then exits QEMU
#else
  Scheduler::start();

  // Parse multiboot info for modules and framebuffer.
  const auto* info = phys_to_virt(paddr_t{mboot_info}).ptr<multiboot_info_t>();
  Modules::init(info);

  // Initialise the framebuffer console before any printf output.
  if (Framebuffer::init(info)) {
    kTerminal.init();
    const auto& fb = Framebuffer::info();
    printf("Framebuffer: %ux%u %ubpp pitch=%u\n", fb.width, fb.height,
           static_cast<unsigned>(fb.bpp), fb.pitch);
  }

  // Set up the VFS with device nodes and module-backed files.
  Vfs::init();
  Vfs::init_devfs();
  Vfs::init_ramfs();

  VfsNode* shell = Vfs::lookup("/bin/sh");
  if ((shell != nullptr) && (shell->data != nullptr)) {
    printf("Loading shell \"%s\" (%u bytes)...\n", shell->name, static_cast<unsigned>(shell->size));
    assert(Scheduler::create_process(std::span<const uint8_t>{shell->data, shell->size},
                                     shell->name) &&
           "kernel_main(): failed to create shell process");
  } else {
    printf("No shell found at /bin/sh\n");
  }

  while (true) {
    __asm__ volatile("hlt");
  }
#endif
  __builtin_unreachable();
}

__END_DECLS
