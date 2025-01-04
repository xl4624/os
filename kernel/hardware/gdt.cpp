#include "gdt.hpp"

#include <stddef.h>

// TODO: Add TSS entry once implemented
static GDTEntry gdt[3];
static GDTDescriptor gdtp;

static GDTEntry create_gdt_entry(size_t base, uint32_t limit, uint8_t access,
                                 uint8_t flags);

extern "C" void gdt_init() {
    gdt[0] = create_gdt_entry(0, 0, 0, 0);             // Null descriptor
    gdt[1] = create_gdt_entry(0, 0xFFFFF, 0x9A, 0xC);  // Code segment
    gdt[2] = create_gdt_entry(0, 0xFFFFF, 0x92, 0xC);  // Data segment

    gdtp.size = (sizeof(GDTEntry) * 3) - 1;
    gdtp.offset = reinterpret_cast<uint32_t>(&gdt);

    // Load the GDTR
    asm("lgdt %0\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        // Far jump to reload CS (code segment)
        // 0x08 is offset into GDT for code segment
        "ljmp $0x08, $1f\n"
        "1:\n"
        "ret\n"
        :
        : "m"(gdtp)
        : "ax"  // Clobbers ax register
    );
}

static GDTEntry create_gdt_entry(size_t base, uint32_t limit, uint8_t access,
                                 uint8_t flags) {
    return {
        .limit_low = static_cast<uint16_t>(limit & 0xFFFF),
        .base_low = static_cast<uint16_t>(base & 0xFFFF),
        .base_mid = static_cast<uint8_t>((base >> 16) & 0xFF),
        .access = access,
        .granularity = static_cast<uint8_t>(((limit >> 16) & 0x0F)
                                            | ((flags << 4) & 0xF0)),
        .base_high = static_cast<uint8_t>((base >> 24) & 0xFF),
    };
}
