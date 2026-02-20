#include "ktest.h"

#include <sys/syscall.h>

#include "address_space.h"
#include "gdt.h"
#include "pmm.h"
#include "process.h"
#include "scheduler.h"
#include "syscall.h"

// ---------------------------------------------------------------------------
// syscall_dispatch
// ---------------------------------------------------------------------------

TEST(syscall, invalid_number) {
    TrapFrame frame = {};
    frame.eax = SYS_MAX + 5;
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

// During ktest the current process is the idle process (PID 0).
TEST(syscall, getpid) {
    TrapFrame frame = {};
    frame.eax = SYS_GETPID;
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, 0u);
}

// The idle process has heap_break == 0, so a zero-increment query returns 0.
TEST(syscall, sbrk_query) {
    TrapFrame frame = {};
    frame.eax = SYS_SBRK;
    frame.ebx = 0;
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, 0u);
}

TEST(syscall, write_bad_fd) {
    TrapFrame frame = {};
    frame.eax = SYS_WRITE;
    frame.ebx = 99;
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

TEST(syscall, read_bad_fd) {
    TrapFrame frame = {};
    frame.eax = SYS_READ;
    frame.ebx = 99;
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

// ---------------------------------------------------------------------------
// Scheduler::alloc_user_stack
// ---------------------------------------------------------------------------

TEST(syscall, alloc_user_stack_range) {
    auto [pd_phys, pd] = AddressSpace::create();
    ASSERT_NOT_NULL(pd);

    uint32_t esp = Scheduler::alloc_user_stack(pd, "test");
    ASSERT_NE(esp, 0u);
    ASSERT_TRUE(esp < static_cast<uint32_t>(kUserStackTop));
    ASSERT_TRUE(esp >= static_cast<uint32_t>(kUserStackVA));

    AddressSpace::destroy(pd, pd_phys);
}

TEST(syscall, alloc_user_stack_pages_are_user_mapped) {
    auto [pd_phys, pd] = AddressSpace::create();
    ASSERT_NOT_NULL(pd);

    uint32_t esp = Scheduler::alloc_user_stack(pd, "hello");
    ASSERT_NE(esp, 0u);
    ASSERT_TRUE(AddressSpace::is_user_mapped(pd, esp, /*writeable=*/true));

    AddressSpace::destroy(pd, pd_phys);
}

// ---------------------------------------------------------------------------
// Scheduler::init_trap_frame
// ---------------------------------------------------------------------------

TEST(syscall, init_trap_frame_fields) {
    TrapFrame frame = {};
    // Poison GP registers to confirm they are all zeroed by init_trap_frame.
    frame.eax = 0xDEAD;
    frame.ebx = 0xBEEF;

    Scheduler::init_trap_frame(&frame, 0x00401000, 0x00BFFFF0);

    ASSERT_EQ(frame.eip, 0x00401000u);
    ASSERT_EQ(frame.user_esp, 0x00BFFFF0u);
    ASSERT_EQ(frame.eflags, 0x202u);
    ASSERT_EQ(frame.cs, static_cast<uint32_t>(GDT::USER_CODE_SELECTOR));
    ASSERT_EQ(frame.user_ss, static_cast<uint32_t>(GDT::USER_DATA_SELECTOR));
    ASSERT_EQ(frame.ds, static_cast<uint32_t>(GDT::USER_DATA_SELECTOR));
    ASSERT_EQ(frame.eax, 0u);
    ASSERT_EQ(frame.ebx, 0u);
    ASSERT_EQ(frame.ecx, 0u);
    ASSERT_EQ(frame.edx, 0u);
    ASSERT_EQ(frame.esi, 0u);
    ASSERT_EQ(frame.edi, 0u);
    ASSERT_EQ(frame.ebp, 0u);
}
