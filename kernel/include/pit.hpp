#pragma once

#include <stdint.h>

/*
 * 8253/8254 Programmable Interval Timer (PIT) driver.
 *
 * Configures PIT channel 0 in rate-generator mode (mode 2) to fire IRQ 0
 * periodically at PIT_TARGET_HZ (100 Hz), giving a tick period of 10 ms.
 * The input clock is 1.193182 MHz; the divisor (11932) is written as two
 * bytes (lo then hi) to the channel 0 data port (0x40).
 *
 * init() must be called after Interrupt::init() so that the IRQ 0 handler
 * can be registered. Interrupts must be enabled for sleep_ticks / sleep_ms
 * to make forward progress (they hlt-wait for each tick).
 */

namespace PIT {
    // Program channel 0 for periodic interrupts and register the IRQ 0
    // handler. Must be called exactly once, after Interrupt::init().
    void init();

    // Return the total number of timer ticks since init().
    uint64_t get_ticks();

    // Sleep for approximately n ticks.
    // Requires interrupts to be enabled.
    void sleep_ticks(uint64_t n);

    // Sleep for approximately the specified number of milliseconds.
    // Requires interrupts to be enabled.
    void sleep_ms(uint32_t ms);
}  // namespace PIT
