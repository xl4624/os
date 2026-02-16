#pragma once

#include <stdint.h>

#include "process.h"

// System call numbers.
static constexpr uint32_t SYS_EXIT = 0;
static constexpr uint32_t SYS_READ = 1;
static constexpr uint32_t SYS_WRITE = 2;
static constexpr uint32_t SYS_SLEEP = 3;
static constexpr uint32_t SYS_MAX = 4;

namespace Syscall {
    // Register IDT entry 0x80 as a user-callable interrupt gate.
    // Must be called after Interrupt::init().
    void init();
}  // namespace Syscall
