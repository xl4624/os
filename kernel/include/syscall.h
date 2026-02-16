#pragma once

#include <stdint.h>

// System call numbers.
static constexpr uint32_t SYS_EXIT = 0;
static constexpr uint32_t SYS_READ = 1;
static constexpr uint32_t SYS_WRITE = 2;
static constexpr uint32_t SYS_SLEEP = 3;
static constexpr uint32_t SYS_MAX = 4;

// Register state saved by syscall_entry.S on the kernel stack.
// Fields are ordered from LOW address (ESP) to HIGH address, which is the
// reverse of the push order: the last thing pushed is at the lowest address.
struct SyscallRegisters {
    // Pushed last by pushal (EDI at lowest address, EAX at highest).
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    // Pushed before pushal: ds pushed last (lowest), gs pushed first (highest).
    uint32_t ds, es, fs, gs;
    // Pushed by CPU on privilege-level change (EIP lowest, SS highest).
    uint32_t eip, cs, eflags, user_esp, user_ss;
} __attribute__((packed));

namespace Syscall {
    // Register IDT entry 0x80 as a user-callable interrupt gate.
    // Must be called after Interrupt::init().
    void init();
}  // namespace Syscall
