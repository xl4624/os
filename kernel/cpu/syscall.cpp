#include "syscall.h"

#include <assert.h>
#include <stdio.h>

#include "idt.h"
#include "interrupt.h"
#include "keyboard.h"
#include "paging.h"
#include "scheduler.h"
#include "tty.h"

static constexpr uint32_t SYSCALL_VECTOR = 0x80;

// ===========================================================================
// Syscall handlers
// ===========================================================================

// Validate that a user pointer range [ptr, ptr+len) is entirely below the
// kernel virtual base. This prevents user code from tricking the kernel into
// reading/writing kernel memory.
// TODO: Also verify the pages are actually mapped and user-accessible (walk the
// page tables) to prevent faults inside the kernel.
static bool validate_user_buffer(uint32_t ptr, uint32_t len) {
    if (len == 0) {
        return true;
    }
    // Overflow check and kernel boundary check.
    return ptr + len > ptr && ptr + len <= KERNEL_VMA;
}

// SYS_WRITE(fd=ebx, buf=ecx, count=edx)
// Returns number of bytes written, or -1 on error.
static int32_t sys_write(TrapFrame *regs) {
    uint32_t fd = regs->ebx;
    uint32_t buf = regs->ecx;
    uint32_t count = regs->edx;

    // TODO: Support stderr (fd=2) and file descriptors once we have a VFS.
    if (fd != 1) {
        return -1;
    }

    if (!validate_user_buffer(buf, count)) {
        return -1;
    }

    const char *data = reinterpret_cast<const char *>(buf);
    for (uint32_t i = 0; i < count; ++i) {
        terminal_putchar(data[i]);
    }

    return static_cast<int32_t>(count);
}

// SYS_READ(fd=ebx, buf=ecx, count=edx)
// Returns number of bytes read, or -1 on error.
static int32_t sys_read(TrapFrame *regs) {
    uint32_t fd = regs->ebx;
    uint32_t buf = regs->ecx;
    uint32_t count = regs->edx;

    // TODO: Support file descriptors once we have a VFS.
    if (fd != 0) {
        return -1;
    }

    if (!validate_user_buffer(buf, count)) {
        return -1;
    }

    char *data = reinterpret_cast<char *>(buf);
    size_t n = kKeyboard.read(data, count);
    return static_cast<int32_t>(n);
}

// SYS_EXIT(code=ebx)
static int32_t sys_exit(TrapFrame *regs) {
    uint32_t code = regs->ebx;
    Scheduler::exit_current(code);
    return 0;  // not reached by the exiting process
}

// SYS_SLEEP(ms=ebx)
static int32_t sys_sleep(TrapFrame *regs) {
    uint32_t ms = regs->ebx;
    Scheduler::sleep_current(ms);
    return 0;
}

// ===========================================================================
// Dispatch table
// ===========================================================================

using syscall_fn = int32_t (*)(TrapFrame *);

static syscall_fn syscall_table[SYS_MAX] = {
    sys_exit,   // 0
    sys_read,   // 1
    sys_write,  // 2
    sys_sleep,  // 3
};

__BEGIN_DECLS

// Assembly entry point defined in syscall_entry.S.
void syscall_entry();

// Called from syscall_entry.S. Returns the kernel ESP to restore (may be a
// different process's stack after exit/sleep triggers a context switch).
uint32_t syscall_dispatch(uint32_t esp) {
    auto *regs = reinterpret_cast<TrapFrame *>(esp);
    assert(regs != nullptr && "syscall_dispatch(): regs pointer is null");
    uint32_t num = regs->eax;

    if (num >= SYS_MAX) {
        regs->eax = static_cast<uint32_t>(-1);
        return Scheduler::schedule(esp);
    }

    int32_t ret = syscall_table[num](regs);
    regs->eax = static_cast<uint32_t>(ret);

    // Run the scheduler. If the syscall blocked or exited the current process,
    // this returns a different process's ESP.
    return Scheduler::schedule(esp);
}

__END_DECLS

// ===========================================================================
// Initialization
// ===========================================================================

namespace Syscall {

    void init() {
        assert(Interrupt::is_initialized()
               && "Syscall::init(): Interrupt must be initialized first");
        IDT::set_entry(SYSCALL_VECTOR,
                       reinterpret_cast<uintptr_t>(syscall_entry),
                       IDT::Gate::Interrupt, IDT::Ring::User);
    }

}  // namespace Syscall
