#include "pic.hpp"

#include <stdint.h>
#include <sys/io.h>

// clang-format off
#define PIC1_CTRL   0x20
#define PIC1_DATA   0x21
#define PIC2_CTRL   0xA0
#define PIC2_DATA   0xA1

#define ICW1_ICW4   0x01
#define ICW1_SINGLE 0x02
#define ICW1_LEVEL  0x08
#define ICW1_INIT   0x10

#define ICW4_8086   0x01

#define IRQ0        32

#define PIC_EOI     0x20
// clang-format on

void pic_init() {
    pic_disable();

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

void pic_disable() {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}
