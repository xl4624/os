#include "interrupt.hpp"

#include <stdio.h>

#include "idt.hpp"
#include "pic.hpp"

static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved15",
    "x87 Floating Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved22",
    "Reserved23",
    "Reserved24",
    "Reserved25",
    "Reserved26",
    "Reserved27",
    "Hypervision Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved31",
};

#define MAKE_EXCEPTION_HANDLER(num)                                           \
    static __attribute__((interrupt)) void isr##num(interrupt_frame *frame) { \
        (void)frame;                                                          \
        printf("%s (#%d)\n", exception_messages[num], num);                   \
        while (1) {                                                           \
            asm volatile("cli; hlt");                                         \
        }                                                                     \
    }

MAKE_EXCEPTION_HANDLER(0)

void interrupt_init() {
    idt_init();
    pic_init();

    // Set up CPU exceptions
    exception_register_handler(ISR::DivideByZero, isr0);

    interrupt_enable();
}

void exception_register_handler(ISR isr, handler_t handler) {
    idt_set_entry(static_cast<uint8_t>(isr), reinterpret_cast<size_t>(handler),
                  0x8E);
}

void interrupt_register_handler(IRQ irq, handler_t handler) {
    pic_unmask(static_cast<uint8_t>(irq));
    idt_set_entry(static_cast<uint8_t>(irq) + 32,
                  reinterpret_cast<uint32_t>(handler), 0x8E);
}

void interrupt_enable() {
    __asm__ volatile("sti");
}

void interrupt_disable() {
    __asm__ volatile("cli");
}
