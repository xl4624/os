#include "idt.hpp"

#include <stdio.h>
#include <string.h>

__attribute__((aligned(0x10))) static IDTEntry idt[256];
static IDTDescriptor idtp;

void idt_init() {
    idtp.size = (sizeof(IDTEntry) * 256) - 1;
    idtp.offset = (uint32_t)&idt;

    // Clear the IDT
    memset(&idt, 0, sizeof(idt));

    // Load the IDTR
    __asm__ volatile("lidt %0" : : "m"(idtp));
}

void idt_set_entry(size_t index, size_t base, uint8_t flags) {
    // TODO: idt_set_entry  should know what flag to put for the index passed
    // in
    idt[index].offset_low = static_cast<uint16_t>(base & 0xFFFF);
    idt[index].selector = static_cast<uint16_t>(0x08);
    idt[index].reserved = 0;
    idt[index].attributes = flags;
    idt[index].offset_high = static_cast<uint16_t>((base >> 16) & 0xFFFF);
}
