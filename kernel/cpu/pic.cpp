#include "pic.hpp"

#include <sys/io.h>

namespace {
    constexpr uint16_t PIC1_CTRL = 0x20;
    constexpr uint16_t PIC1_DATA = 0x21;
    constexpr uint16_t PIC2_CTRL = 0xA0;
    constexpr uint16_t PIC2_DATA = 0xA1;

    constexpr uint16_t ICW1_INIT = 0x10;
    constexpr uint16_t ICW1_ICW4 = 0x01;
    constexpr uint16_t ICW4_8086 = 0x01;

    constexpr uint16_t PIC_EOI = 0x20;
    constexpr uint16_t IRQ0 = 32;
}  // namespace

namespace PIC {
    void init() {
        // All IRQs start masked, get unmasked on interrupt_register_handler()
        disable();

        // ICW1: Start initialization sequence in cascade mode
        outb(PIC1_CTRL, ICW1_INIT | ICW1_ICW4);
        io_wait();
        outb(PIC2_CTRL, ICW1_INIT | ICW1_ICW4);
        io_wait();

        // ICW2: Remap IRQs to interrupts 32-47
        outb(PIC1_DATA, IRQ0);  // Master IRQs 0-7 mapped to interrupts 32-39
        io_wait();
        outb(PIC2_DATA,
             IRQ0 + 8);  // Slave IRQs 8-15 mapped to interrupts 40-47
        io_wait();

        // ICW3: Tell PICs how they're cascaded
        outb(PIC1_DATA, 4);
        io_wait();
        outb(PIC2_DATA, 2);
        io_wait();

        // ICW4: Set 8086 mode
        outb(PIC1_DATA, ICW4_8086);
        io_wait();
        outb(PIC2_DATA, ICW4_8086);
        io_wait();
    }

    void sendEOI(uint8_t irq) {
        if (irq >= 8)
            outb(PIC2_CTRL, PIC_EOI);
        outb(PIC1_CTRL, PIC_EOI);
    }

    void mask(uint8_t irq) {
        uint16_t port;
        uint8_t value;

        if (irq < 8) {
            port = PIC1_DATA;
        } else {
            port = PIC2_DATA;
            irq -= 8;
        }
        value = inb(port) | (1 << irq);
        outb(port, value);
    }

    void unmask(uint8_t irq) {
        uint16_t port;
        uint8_t value;

        if (irq < 8) {
            port = PIC1_DATA;
        } else {
            port = PIC2_DATA;
            irq -= 8;
        }
        value = inb(port) & ~(1 << irq);
        outb(port, value);
    }

    void disable() {
        outb(PIC1_DATA, 0xFF);
        outb(PIC2_DATA, 0xFF);
    }
}  // namespace PIC
