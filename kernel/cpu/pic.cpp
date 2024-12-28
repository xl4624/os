#include "pic.hpp"

#include <stdint.h>
#include <sys/io.h>

#include "isr.hpp"

extern "C" void pic_init() {
    uint8_t a1, a2;

    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    outb(PIC1_CTRL, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_CTRL, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, IRQ0);
    io_wait();
    outb(PIC2_DATA, IRQ0 + 8);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);

    pic_mask(static_cast<uint8_t>(ISR::IRQ::Timer));
    pic_unmask(static_cast<uint8_t>(ISR::IRQ::Keyboard));
}

void pic_sendEOI(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_CTRL, PIC_EOI);
    outb(PIC1_CTRL, PIC_EOI);
}

void pic_mask(uint8_t irq) {
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

void pic_unmask(uint8_t irq) {
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
