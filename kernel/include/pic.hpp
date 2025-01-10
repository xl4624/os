#pragma once

#include <stdint.h>

namespace PIC {
    void init();
    void sendEOI(uint8_t irq);
    void mask(uint8_t irq);
    void unmask(uint8_t irq);
    void disable();
}  // namespace PIC
