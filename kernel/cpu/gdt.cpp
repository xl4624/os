#include "gdt.h"

#include <assert.h>
#include <stddef.h>

#include "tss.h"

namespace GDT {
// 6 entries: null, kernel code, kernel data, TSS, user code, user data
static Entry gdt[6];
static Descriptor gdtp;
static bool initialized = false;

static constexpr Entry create_gdt_entry(size_t base, uint32_t limit, uint8_t access,
                                        uint8_t flags) {
  return {
      /*.limit_low=*/static_cast<uint16_t>(limit & 0xFFFF),
      /*.base_low=*/static_cast<uint16_t>(base & 0xFFFF),
      /*.base_mid=*/static_cast<uint8_t>((base >> 16) & 0xFF),
      /*.access=*/access,
      /*.granularity=*/static_cast<uint8_t>(((limit >> 16) & 0x0F) | ((flags << 4) & 0xF0)),
      /*.base_high=*/static_cast<uint8_t>((base >> 24) & 0xFF),
  };
}

void init() {
  assert(!initialized && "GDT::init(): called more than once");

  // Null descriptor
  gdt[0] = create_gdt_entry(/*base=*/0, /*limit=*/0, /*access=*/0, /*flags=*/0);
  // Kernel code segment (ring 0)
  gdt[1] = create_gdt_entry(/*base=*/0, /*limit=*/SEGMENT_LIMIT, /*access=*/CODE_ACCESS,
                            /*flags=*/FLAGS_4K_32BIT);
  // Kernel data segment (ring 0)
  gdt[2] = create_gdt_entry(/*base=*/0, /*limit=*/SEGMENT_LIMIT, /*access=*/DATA_ACCESS,
                            /*flags=*/FLAGS_4K_32BIT);
  // TSS descriptor - base = address of TSS struct, limit = size-1 bytes,
  // flags = 0 (byte granularity, no size/granularity bits for system seg)
  gdt[3] = create_gdt_entry(/*base=*/reinterpret_cast<size_t>(&TSS::tss),
                            /*limit=*/sizeof(TSS::Entry) - 1, /*access=*/TSS_ACCESS,
                            /*flags=*/0x0);
  // User code segment (ring 3)
  gdt[4] = create_gdt_entry(/*base=*/0, /*limit=*/SEGMENT_LIMIT, /*access=*/USER_CODE_ACCESS,
                            /*flags=*/FLAGS_4K_32BIT);
  // User data segment (ring 3)
  gdt[5] = create_gdt_entry(/*base=*/0, /*limit=*/SEGMENT_LIMIT, /*access=*/USER_DATA_ACCESS,
                            /*flags=*/FLAGS_4K_32BIT);

  gdtp.size = (sizeof(Entry) * 6) - 1;
  gdtp.offset = reinterpret_cast<uintptr_t>(&gdt);

  // Load the GDTR
  asm volatile(
      "lgdt %0\n"
      "mov %1, %%ax\n"
      "mov %%ax, %%ds\n"
      "mov %%ax, %%es\n"
      "mov %%ax, %%fs\n"
      "mov %%ax, %%gs\n"
      "mov %%ax, %%ss\n"
      "pushl %2\n"
      "pushl $1f\n"
      "lret\n"
      "1:\n"
      :
      : "m"(gdtp), "i"(KERNEL_DATA_SELECTOR), "i"(KERNEL_CODE_SELECTOR)
      : "ax", "memory");

  initialized = true;
}

bool is_initialized() { return initialized; }
}  // namespace GDT
