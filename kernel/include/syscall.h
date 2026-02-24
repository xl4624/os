#pragma once

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/syscall.h>

#include "process.h"

namespace Syscall {

// Register IDT entry 0x80 as a user-callable interrupt gate.
// Must be called after Interrupt::init().
void init();

}  // namespace Syscall

__BEGIN_DECLS

// Called from syscall_entry.S. Dispatches the syscall identified by
// regs->eax and returns the kernel ESP to restore (which may be a different
// process after a blocking syscall triggers a context switch).
uint32_t syscall_dispatch(uint32_t esp);

__END_DECLS
