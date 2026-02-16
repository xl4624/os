#include "usermode.h"

#include <assert.h>

#include "gdt.h"
#include "paging.h"

namespace Usermode {

    [[noreturn]]
    void jump(uint32_t entry, uint32_t user_stack_top) {
        assert(entry != 0 && "Usermode::jump(): entry point is null");
        assert(entry < KERNEL_VMA
               && "Usermode::jump(): entry point is in kernel address space");
        assert(user_stack_top != 0
               && "Usermode::jump(): user stack top is zero");
        assert(
            user_stack_top <= KERNEL_VMA
            && "Usermode::jump(): user stack top is in kernel address space");
        asm volatile(
            // Load user data segment selectors into ds, es, fs, gs.
            "mov %[uds], %%ax\n"
            "mov %%ax, %%ds\n"
            "mov %%ax, %%es\n"
            "mov %%ax, %%fs\n"
            "mov %%ax, %%gs\n"

            // Build the iret frame on the kernel stack:
            //   SS     = user data selector
            //   ESP    = user stack top
            //   EFLAGS = 0x202 (IF set, reserved bit 1 set)
            //   CS     = user code selector
            //   EIP    = user entry point
            "push %[uds]\n"
            "push %[esp]\n"
            "push $0x202\n"
            "push %[ucs]\n"
            "push %[eip]\n"
            "iret\n"
            :
            : [uds] "i"(GDT::USER_DATA_SELECTOR),
              [ucs] "i"(GDT::USER_CODE_SELECTOR), [esp] "r"(user_stack_top),
              [eip] "r"(entry)
            : "eax", "memory");

        __builtin_unreachable();
    }

}  // namespace Usermode
