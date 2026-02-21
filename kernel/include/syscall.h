#pragma once

#include <stdint.h>
#include <sys/syscall.h>

#include "process.h"

namespace Syscall {

    // Register IDT entry 0x80 as a user-callable interrupt gate.
    // Must be called after Interrupt::init().
    void init();

}  // namespace Syscall
