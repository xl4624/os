#include "idt.hpp"

#include <string.h>

#include "isr.hpp"

IDTEntry idt[256];
IDTDescriptor idtp;

extern "C" void idt_init() {
    idtp.size = (sizeof(IDTEntry) * 256) - 1;
    idtp.offset = (uint32_t)&idt;

    // Clear the IDT
    memset(&idt, 0, sizeof(idt));

    idt[0] =
        create_idt_entry(reinterpret_cast<uint32_t>(ISR::divide_by_zero), 0x8E);
    // TODO: Setup CPU exceptions (0-31)

    idt[33] = create_idt_entry(reinterpret_cast<uint32_t>(ISR::keyboard), 0x8E);
    // TODO: Set up IRQs (32-47)

    __asm__ volatile("lidt %0" : : "m"(idtp));
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
