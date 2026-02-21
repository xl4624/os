#pragma once

#include <stdint.h>

/*
 * 8259 Programmable Interrupt Controller (PIC) driver.
 *
 * The PC/AT has two cascaded 8259A PICs: a master (IRQs 0–7) and a slave
 * (IRQs 8–15) connected through the master's IRQ 2 line. By default they
 * deliver interrupts on vectors 8–15 and 112–119, which overlap CPU exception
 * vectors. init() remaps them to vectors 32–47 (IRQ 0 → vector 32, etc.).
 *
 * All IRQs start masked after init(). register_handler() in interrupt.cpp
 * calls unmask() automatically when a handler is installed.
 */

namespace PIC {

    // Remap both PICs so IRQs 0–15 arrive on vectors 32–47, then mask all
    // IRQ lines. Must be called before enabling interrupts.
    void init();

    // Signal end-of-interrupt to the PIC(s). Must be called at the end of
    // every IRQ handler. Sends EOI to the slave as well if irq >= 8.
    void send_eoi(uint8_t irq);

    // Suppress interrupts from the given IRQ line (0–15).
    void mask(uint8_t irq);

    // Allow interrupts from the given IRQ line (0–15).
    void unmask(uint8_t irq);

    // Mask all IRQ lines on both PICs (write 0xFF to both data ports).
    void disable();

}  // namespace PIC
