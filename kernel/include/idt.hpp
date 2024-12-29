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
 *
 * Offset (63-48, 15-0): 32-bit address of the interrupt handler routine.
 *
 * Type (36-32): Gate type and attributes
 *   - 0b00101 (0x5): Task Gate
 *   - 0b01110 (0xE): Interrupt Gate
 *   - 0b01111 (0xF): Trap Gate
 *
 * Segment Selector (31-16): Code segment selector in GDT/LDT
 *
 * Attributes (47-40):
 *   - P (bit 47): Present bit
 *   - DPL (bits 46-45): Descriptor Privilege Level
 *      00: Ring 0 (Kernel)
 *      01: Ring 1
 *      10: Ring 2
 *      11: Ring 3 (User)
 *   - S (bit 44): Storage Segment (must be 0 for IDT)
 *
 * Reserved (bits 43-40): Must be 0
 */
struct [[gnu::packed]] IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t reserved;
    uint8_t attributes;
    uint16_t offset_high;
};

struct [[gnu::packed]] IDTDescriptor {
    uint16_t size;
    uint32_t offset;
};

// TODO: Define some enums that might be useful, task gate interrupt gate and
// trap gate

void idt_init();

void idt_set_entry(size_t index, size_t base, uint8_t flags);
