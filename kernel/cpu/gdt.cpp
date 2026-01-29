#include "gdt.hpp"

#include <assert.h>
#include <stddef.h>

namespace GDT {
    // TODO: Add TSS entry once implemented
    static Entry gdt[3];
    static Descriptor gdtp;
    static bool initialized = false;

    static constexpr Entry create_gdt_entry(size_t base, uint32_t limit,
                                            uint8_t access, uint8_t flags) {
        return {
            /*.limit_low=*/static_cast<uint16_t>(limit & 0xFFFF),
            /*.base_low=*/static_cast<uint16_t>(base & 0xFFFF),
            /*.base_mid=*/static_cast<uint8_t>((base >> 16) & 0xFF),
            /*.access=*/access,
            /*.granularity=*/
            static_cast<uint8_t>(((limit >> 16) & 0x0F)
                                 | ((flags << 4) & 0xF0)),
            /*.base_high=*/static_cast<uint8_t>((base >> 24) & 0xFF),
        };
    }

    void init() {
        assert(!initialized && "GDT::init(): called more than once");

        // Null descriptor
        gdt[0] = create_gdt_entry(0, 0, 0, 0);
        // Code segment
        gdt[1] =
            create_gdt_entry(0, SEGMENT_LIMIT, CODE_ACCESS, FLAGS_4K_32BIT);
        // Data segment
        gdt[2] =
            create_gdt_entry(0, SEGMENT_LIMIT, DATA_ACCESS, FLAGS_4K_32BIT);

        gdtp.size = (sizeof(Entry) * 3) - 1;
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
