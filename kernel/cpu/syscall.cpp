#include "syscall.h"

#include <assert.h>
#include <stdio.h>

#include "address_space.h"
#include "idt.h"
#include "interrupt.h"
#include "keyboard.h"
#include "paging.h"
#include "pmm.h"
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

// SYS_SBRK(increment=ebx)
// Returns old break on success, or (uint32_t)-1 on failure.
static int32_t sys_sbrk(TrapFrame *regs) {
    auto increment = static_cast<int32_t>(regs->ebx);
    Process *proc = Scheduler::current();

    vaddr_t old_break = proc->heap_break;

    if (increment == 0) {
        return static_cast<int32_t>(old_break);
    }

    vaddr_t new_break = old_break + static_cast<uint32_t>(increment);

    // Reject if the new break would enter kernel space or wrap around.
    if (new_break >= KERNEL_VMA || new_break < old_break) {
        return -1;
    }

    if (increment > 0) {
        // Map any new pages between old_break and new_break.
        vaddr_t old_page = (old_break + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        vaddr_t new_page = (new_break + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

        for (vaddr_t va = old_page; va < new_page; va += PAGE_SIZE) {
            paddr_t phys = kPmm.alloc();
            if (phys == 0) {
                // TODO: unmap and free here.

                // for (vaddr_t undo = old_page; undo < va; undo += PAGE_SIZE)
                // {}
                return -1;
            }
            AddressSpace::map(proc->page_directory, va, phys,
                              /*writeable=*/true, /*user=*/true);

            // Zero the page so userspace gets clean memory.
            auto *page = reinterpret_cast<uint8_t *>(phys_to_virt(phys));
            __builtin_memset(page, 0, PAGE_SIZE);
        }
    }

    proc->heap_break = new_break;
    return static_cast<int32_t>(old_break);
}

// SYS_SET_CURSOR(row=ebx, col=ecx)
static int32_t sys_set_cursor(TrapFrame *regs) {
    uint32_t row = regs->ebx;
    uint32_t col = regs->ecx;
    terminal_set_position(row, col);
    return 0;
}

// SYS_SET_COLOR(color=ebx)
static int32_t sys_set_color(TrapFrame *regs) {
    uint32_t color = regs->ebx;
    terminal_set_color(static_cast<uint8_t>(color));
    return 0;
}

// SYS_CLEAR()
static int32_t sys_clear([[maybe_unused]] TrapFrame *regs) {
    terminal_clear();
    return 0;
}

// ===========================================================================
// Dispatch table
// ===========================================================================

using syscall_fn = int32_t (*)(TrapFrame *);

static syscall_fn syscall_table[SYS_MAX] = {
    sys_exit,        // 0
    sys_read,        // 1
    sys_write,       // 2
    sys_sleep,       // 3
    sys_sbrk,        // 4
    sys_set_cursor,  // 5
    sys_set_color,   // 6
    sys_clear,       // 7
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
