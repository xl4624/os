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
    enum class Gate : uint8_t {
        Task = 0x5,
        Interrupt = 0xE,
        Trap = 0xF,
    };

    /* Modern systems only use Ring 0 (kernel) and Ring 3 (user) */
    enum class Ring : uint8_t {
        Kernel = 0,
        System = 1,
        Privileged = 2,
        User = 3,
    };

    struct Entry {
        uint16_t offset_low;  // Lower 16 bits of handler address
        uint16_t segment;     // Kernel segment selector
        uint8_t reserved;     // Must be 0
        struct Attributes {
            uint8_t gate_type : 4;
            uint8_t zero : 1;  // Must be 0
            Ring dpl : 2;      // Privilege levels
            uint8_t present : 1;

            Attributes() = default;
            constexpr Attributes(Gate gate, Ring ring)
                : gate_type(static_cast<uint8_t>(gate)),
                  zero(0),
                  dpl(ring),
                  present(1) {}
        } __attribute__((packed)) attributes;

        uint16_t offset_high;  // Upper 16 bits of handler address

        Entry() = default;
        constexpr Entry(uintptr_t handler, Gate gate, Ring ring)
            : offset_low(static_cast<uint16_t>(handler & 0xFFFF)),
              segment(static_cast<uint16_t>(0x08)),  // TODO: GDT::KernelCode
              reserved(0),
              attributes(gate, ring),
              offset_high(static_cast<uint16_t>((handler >> 16) & 0xFFFF)) {}
    } __attribute__((packed));

    struct Descriptor {
        uint16_t size;
        uintptr_t offset;
    } __attribute__((packed));

    void init();
    void set_entry(size_t index, uintptr_t handler, Gate gate, Ring ring);
}  // namespace IDT
