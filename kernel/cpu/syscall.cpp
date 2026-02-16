#include "syscall.h"

#include <assert.h>
#include <stdio.h>

#include "gdt.h"
#include "idt.h"
#include "interrupt.h"
#include "keyboard.h"
#include "paging.h"
#include "pit.h"
#include "tty.h"

// Assembly entry point defined in syscall_entry.S.
extern "C" void syscall_entry();

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
static int32_t sys_write(SyscallRegisters *regs) {
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
static int32_t sys_read(SyscallRegisters *regs) {
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
// TODO: Mark process as zombie and schedule next process once we have a process
// scheduler (Milestone 4). Currently just halts.
static int32_t sys_exit(SyscallRegisters *regs) {
    uint32_t code = regs->ebx;
    printf("Process exited with code %u\n", code);
    halt_and_catch_fire();
}

// SYS_SLEEP(ms=ebx)
// TODO: Block the process and schedule next once we have a scheduler. Currently
// busy-waits.
static int32_t sys_sleep(SyscallRegisters *regs) {
    uint32_t ms = regs->ebx;
    PIT::sleep_ms(ms);
    return 0;
}

// ===========================================================================
// Dispatch table
// ===========================================================================

using syscall_fn = int32_t (*)(SyscallRegisters *);

static syscall_fn syscall_table[SYS_MAX] = {
    sys_exit,   // 0
    sys_read,   // 1
    sys_write,  // 2
    sys_sleep,  // 3
};

extern "C" void syscall_dispatch(SyscallRegisters *regs) {
    assert(regs != nullptr && "syscall_dispatch(): regs pointer is null");
    uint32_t num = regs->eax;

    if (num >= SYS_MAX) {
        regs->eax = static_cast<uint32_t>(-1);
        return;
    }

    int32_t ret = syscall_table[num](regs);
    regs->eax = static_cast<uint32_t>(ret);
}

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
