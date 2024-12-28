#include "idt.hpp"

#include <string.h>

#include "isr.hpp"

IDTEntry idt[256];
IDTDescriptor idtp;

void idt_init() {
    idtp.size = (sizeof(IDTEntry) * 256) - 1;
    idtp.offset = (uint32_t)&idt;
    memset(&idt, 0, sizeof(IDTEntry) * 256);

    idt[0] = create_idt_entry(reinterpret_cast<uint32_t>(isr0_handler), 0x8E);

    __asm__ volatile("lidt %0" : : "m"(idtp));
    __asm__ volatile("sti");
}

IDTEntry create_idt_entry(uint32_t base, uint8_t flags) {
    return {
        .offset_low = static_cast<uint16_t>(base & 0xFFFF),
        .selector = static_cast<uint16_t>(0x08),
        .reserved = 0,
        .attributes = flags,
        .offset_high = static_cast<uint16_t>((base >> 16) & 0xFFFF),
    };
}
