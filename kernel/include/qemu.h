#pragma once

#include <stdint.h>

// isa-debug-exit device: QEMU exits with code (value << 1) | 1 when the guest
// writes to this port. Configured in the Makefile with iobase=0xf4,iosize=0x04.
namespace Qemu {

// isa-debug-exit device: QEMU exits with code (value << 1) | 1 when the guest
// writes to this port. Configured in the Makefile with iobase=0xf4,iosize=0x04.
static constexpr uint16_t kDebugExitPort = 0xF4;

// Exit QEMU with the given status code. The actual QEMU exit code will be
// (status << 1) | 1, so status=0 -> exit code 1, status=1 -> exit code 3, etc.
[[noreturn]] static inline void exit(uint8_t status) {
  __asm__ volatile("outb %0, %1" ::"a"(status), "Nd"(kDebugExitPort));
  __builtin_unreachable();
}

}  // namespace Qemu
