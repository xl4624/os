#include "tss.h"

#include <assert.h>
#include <string.h>

#include "gdt.h"

namespace TSS {
Entry tss;

static bool initialized = false;

void init() {
  assert(!initialized && "TSS::init(): called more than once");
  assert(GDT::is_initialized() && "TSS::init(): GDT not yet initialised");

  memset(&tss, 0, sizeof(Entry));

  // When an interrupt fires in ring-3, the CPU switches to the ring-0
  // stack described here. set_kernel_stack() updates esp0 before every
  // ring-3 entry; the initial value is set by kernel_init().
  tss.ss0 = GDT::KERNEL_DATA_SELECTOR;

  // Setting iomap_base past the end of the TSS tells the CPU there is no
  // I/O permission bitmap, so all port I/O from ring-3 will fault.
  tss.iomap_base = sizeof(Entry);

  // Load the Task Register. The GDT descriptor for the TSS was installed
  // by GDT::init() at selector TSS_SELECTOR.
  asm volatile("ltr %0" ::"rm"(static_cast<uint16_t>(GDT::TSS_SELECTOR)) : "memory");

  initialized = true;
}

void set_kernel_stack(uint32_t esp0) {
  assert(initialized && "TSS::set_kernel_stack(): TSS not yet initialized");
  assert(esp0 != 0 && "TSS::set_kernel_stack(): esp0 is zero");
  tss.esp0 = esp0;
}

}  // namespace TSS
