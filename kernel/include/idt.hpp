#pragma once

#include <stddef.h>
#include <stdint.h>

/*
 * ==========================================
 *      Interrupt Descriptor Table (IDT)
 * ==========================================
 *
 * An IDT entry is an 8-byte structure used by the CPU to handle interrupts
 * and exceptions.
 *
 * Bit Layout:
 * 63  48 | 47 | 46 45 | 44 | 43     40 | 39    32 | 31    16 | 15   0
 * Offset | P  | DPL   | S  | Gate Type | Reserved | Selector | Offset
 * 31-16  | 1  | 1-0   | 0  | 3-0       |          | 15-0     | 15-0
 */
namespace IDT {
    enum Gate : uint8_t {
        Task = 0x5,
        Interrupt = 0xE,
        Trap = 0xF,
    };

    enum Ring : uint8_t {
        Kernel = 0,
        Driver = 1,
        Device = 2,
        User = 3,
    };

    constexpr uint8_t make_attr(Ring ring, Gate type) {
        return (1 << 7) | (ring << 5) | type;
    }

    constexpr uint8_t KERN_INT = make_attr(Ring::Kernel, Gate::Interrupt);
    constexpr uint8_t KERN_TRAP = make_attr(Ring::Kernel, Gate::Trap);
    constexpr uint8_t USER_INT = make_attr(Ring::User, Gate::Interrupt);
    constexpr uint8_t USER_TRAP = make_attr(Ring::User, Gate::Trap);

    struct [[gnu::packed]] Entry {
        uint16_t offset_low;
        uint16_t selector;
        uint8_t reserved;
        uint8_t attributes;
        uint16_t offset_high;
    };

    struct [[gnu::packed]] Descriptor {
        uint16_t size;
        uint32_t offset;
    };

    void init();
    void set_entry(size_t index, size_t base, uint8_t flags);
}  // namespace IDT
