#pragma once

#include <stdint.h>

/*
 * =======================================
 *      Task State Segment (TSS)
 * =======================================
 *
 * The 32-bit TSS is a 104-byte hardware structure used by the CPU to find
 * the kernel stack (ESP0, SS0) when an interrupt arrives while the CPU is
 * running in ring-3 (user mode). We use software task switching, so the
 * other fields (saved register state, CR3, etc.) are unused.
 *
 * Layout follows Intel SDM Vol. 3 §8.2.1 - 32-bit TSS.
 */

namespace TSS {
struct Entry {
  uint16_t link;
  uint16_t reserved0;  // 0: Previous Task Link (unused)
  uint32_t esp0;       // 4: Ring-0 stack pointer
  uint16_t ss0;
  uint16_t reserved1;  // 8: Ring-0 stack segment
  uint32_t esp1;       // 12: Ring-1 (unused)
  uint16_t ss1;
  uint16_t reserved2;  // 16
  uint32_t esp2;       // 20: Ring-2 (unused)
  uint16_t ss2;
  uint16_t reserved3;  // 24
  uint32_t cr3;        // 28: (unused - we manage CR3 directly)
  uint32_t eip;        // 32
  uint32_t eflags;     // 36
  uint32_t eax;        // 40
  uint32_t ecx;        // 44
  uint32_t edx;        // 48
  uint32_t ebx;        // 52
  uint32_t esp;        // 56
  uint32_t ebp;        // 60
  uint32_t esi;        // 64
  uint32_t edi;        // 68
  uint16_t es;
  uint16_t reserved4;  // 72
  uint16_t cs;
  uint16_t reserved5;  // 76
  uint16_t ss;
  uint16_t reserved6;  // 80
  uint16_t ds;
  uint16_t reserved7;  // 84
  uint16_t fs;
  uint16_t reserved8;  // 88
  uint16_t gs;
  uint16_t reserved9;  // 92
  uint16_t ldtr;
  uint16_t reserved10;  // 96: LDT selector (unused)
  uint16_t trap;        // 100: Debug trap flag
  uint16_t iomap_base;  // 102: I/O permission bitmap offset
} __attribute__((packed));

static_assert(sizeof(Entry) == 104, "TSS must be 104 bytes");

// The single kernel TSS instance. Its address is installed in the GDT by
// GDT::init(); TSS::init() configures the fields and loads the selector.
extern Entry tss;

// Configure the TSS (ss0, iomap_base) and load the task register.
// Must be called after GDT::init().
void init();

// Update the ring-0 stack pointer stored in the TSS.
// Call on every context switch to ensure ring-3 interrupts land on the
// correct kernel stack.
void set_kernel_stack(uint32_t esp0);

}  // namespace TSS
