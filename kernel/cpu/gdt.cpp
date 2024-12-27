#include "gdt.hpp"

// TODO: Add TSS entry once implemented
static GDTEntry gdt[3];
GDTDescriptor gdtp;

static GDTEntry create_gdt_entry(uint32_t base, uint32_t limit, uint8_t access,
                               uint8_t flags);

extern "C" void gdt_init() {
    gdt[0] = create_gdt_entry(0, 0, 0, 0);             // Null descriptor
    gdt[1] = create_gdt_entry(0, 0xFFFFF, 0x9A, 0xC);  // Code segment
    gdt[2] = create_gdt_entry(0, 0xFFFFF, 0x92, 0xC);  // Data segment

    gdtp.size = (3 * sizeof(GDTEntry)) - 1;
    gdtp.offset = reinterpret_cast<uint32_t>(&gdt);

    load_gdt();
}

static GDTEntry create_gdt_entry(uint32_t base, uint32_t limit, uint8_t access,
                               uint8_t flags) {
    GDTEntry entry;

    entry.limit_low = static_cast<uint16_t>(limit & 0xFFFF);
    entry.base_low = static_cast<uint16_t>(base & 0xFFFF);
    entry.base_mid = static_cast<uint8_t>((base >> 16) & 0xFF);
    entry.access = access;
    entry.granularity =
        static_cast<uint8_t>(((limit >> 16) & 0x0F) | ((flags << 4) & 0xF0));
    entry.base_high = static_cast<uint8_t>((base >> 24) & 0xFF);

    return entry;
}
