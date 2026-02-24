#include "pic.h"

#include <assert.h>
#include <sys/io.h>

static constexpr uint16_t PIC1_CTRL = 0x20;
static constexpr uint16_t PIC1_DATA = 0x21;
static constexpr uint16_t PIC2_CTRL = 0xA0;
static constexpr uint16_t PIC2_DATA = 0xA1;

static constexpr uint8_t ICW1_INIT = 0x10;
static constexpr uint8_t ICW1_ICW4 = 0x01;
static constexpr uint8_t ICW4_8086 = 0x01;

static constexpr uint8_t PIC_EOI = 0x20;
static constexpr uint8_t IRQ0 = 32;

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
  static constexpr uint8_t MASTER_CASCADE_IRQ = 4;
  static constexpr uint8_t SLAVE_CASCADE_IDENTITY = 2;
  outb(PIC1_DATA, MASTER_CASCADE_IRQ);
  io_wait();
  outb(PIC2_DATA, SLAVE_CASCADE_IDENTITY);
  io_wait();

  // ICW4: Set 8086 mode
  outb(PIC1_DATA, ICW4_8086);
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();
}

void send_eoi(uint8_t irq) {
  assert(irq <= 15 && "PIC::send_eoi(): irq out of range (0–15)");
  if (irq >= 8) {
    outb(PIC2_CTRL, PIC_EOI);
  }
  outb(PIC1_CTRL, PIC_EOI);
}

void mask(uint8_t irq) {
  assert(irq <= 15 && "PIC::mask(): irq out of range (0–15)");
  uint16_t port;
  uint8_t value;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    irq = static_cast<uint8_t>(irq - 8u);
  }
  value = static_cast<uint8_t>(inb(port) | (1U << irq));
  outb(port, value);
}

void unmask(uint8_t irq) {
  assert(irq <= 15 && "PIC::unmask(): irq out of range (0–15)");
  uint16_t port;
  uint8_t value;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    irq = static_cast<uint8_t>(irq - 8u);
  }
  value = static_cast<uint8_t>(inb(port) & static_cast<uint8_t>(~(1U << irq)));
  outb(port, value);
}

void disable() {
  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);
}
}  // namespace PIC
