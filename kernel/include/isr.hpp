#pragma once

#include <stdint.h>

extern "C" {
struct interrupt_frame;
}
namespace ISR {
    enum class Exception : uint8_t {
        DivideByZero = 0,
    };

    enum class IRQ : uint8_t {
        Timer = 0,
        Keyboard = 1,
    };

    __attribute__((interrupt)) void divide_by_zero(interrupt_frame *frame);

    __attribute__((interrupt)) void keyboard(interrupt_frame *frame);
}  // namespace ISR
