#include "idt.h"

#include <array.h>
#include <assert.h>
#include <string.h>

namespace IDT {

static constexpr size_t kEntryCount = 256;

__attribute__((aligned(0x10))) static std::array<Entry, kEntryCount> idt;
static Descriptor idtp;

void init() {
  idtp.size = sizeof(idt) - 1;
  idtp.offset = reinterpret_cast<uintptr_t>(idt.data());
  memset(idt.data(), 0, sizeof(idt));
  asm volatile("lidt %0" : : "m"(idtp));
}

void set_entry(size_t index, uintptr_t handler, Gate gate, Ring ring) {
  assert(index < kEntryCount && "IDT::set_entry(): index out of range (0-255)");
  idt[index] = Entry(handler, gate, ring);
}

}  // namespace IDT
