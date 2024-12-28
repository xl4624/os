#pragma once

#include <stdint.h>

// clang-format off
#define PIC1_CTRL      0x20
#define PIC1_DATA      0x21
#define PIC2_CTRL      0xA0
#define PIC2_DATA      0xA1

#define ICW1_ICW4      0x01
#define ICW1_SINGLE    0x02
#define ICW1_INTERVAL4 0x04
#define ICW1_LEVEL     0x08
#define ICW1_INIT      0x10

#define ICW4_8086      0x01

#define IRQ0           32
#define IRQ1           33

#define PIC_EOI        0x20
// clang-format on

extern "C" void pic_init();
void pic_sendEOI(uint8_t irq);
void pic_mask(uint8_t irq);
void pic_unmask(uint8_t irq);
