#pragma once

#include <stdint.h>

/*
 * =======================================
 *      Global Descriptor Table (GDT)
 * =======================================
 *
 * A GDT entry is an 8-byte structure defining memory segments.
 *
 * Bit Layout:
 * 63 56 | 55 52 | 51 48 | 47  40 | 39 32 | 31 16 | 15  0
 * Base  | Flags | Limit | Access | Base  | Base  | Limit
 * 31-24 | 3-0   | 19-16 | 7-0    | 23-16 | 15-0  | 15-0
 *
 * Base (63–56, 39–32, 41–16): 32-bit base address of the segment.
 *
 * Limit (51-48, 15–0): 20-bit value describing maximum addressable unit
 * (scaled by page granularity).
 *
 * Access Byte (47-40):
 *   - P (bit 47): Present bit (must be 1 for valid selectors)
 *   - DPL (bits 46-45): Descriptor Privilege Level
 *      00: Ring 0 (Kernel)
 *      01: Ring 1
 *      10: Ring 2
 *      11: Ring 3 (User)
 *   - S (bit 44): System Descriptor
 *      0: System Segment (TSS, LDT)
 *      1: Code or Data Segment
 *   - E (bit 43): Executable bit
 *      0: Data Segment
 *      1: Code Segment
 *   - DC (bit 42): Direction/Conforming
 *      Data (E=0):
 *        0: Segment grows up
 *        1: Segment grows down
 *      Code (E=1):
 *        0: Can only be executed from ring set in DPL
 *        1: Can be executed from rings ≥ DPL
 *   - RW (bit 41): Read/Write
 *      Data (E=0): Writable bit
 *        0: Read-only
 *        1: Read/write
 *      Code (E=1): Readable bit
 *        0: Execute-only
 *        1: Execute/read
 *   - A (bit 40): Accessed (set by CPU)
 *
 * Flags (55-52):
 *   - G (bit 55): Granularity
 *      0: Limit is in 1 byte blocks
 *      1: Limit is in 4KiB blocks
 *   - DB (bit 54): Size flag
 *      0: 16-bit protected mode segment
 *      1: 32-bit protected mode segment
 *   - L (bit 53): Long-mode code flag
 *      0: Protected mode or compatibility mode segment
 *      1: 64-bit mode segment
 *   - Reserved (bit 52): Must be 0
 */
namespace GDT {
    struct Entry {
        uint16_t limit_low;
        uint16_t base_low;
        uint8_t base_mid;
        uint8_t access;
        uint8_t granularity;
        uint8_t base_high;
    } __attribute__((packed));

    struct Descriptor {
        uint16_t size;    // Size of GDT in bytes - 1: (8 * entry) - 1
        uintptr_t offset;  // GDT linear address
    } __attribute__((packed));

    void init();
}  // namespace GDT
