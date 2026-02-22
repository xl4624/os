#include <sys/syscall.h>

#include "address_space.h"
#include "gdt.h"
#include "ktest.h"
#include "paging.h"
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

// ---------------------------------------------------------------------------
// SYS_WRITE
// ---------------------------------------------------------------------------

TEST(syscall, write_bad_fd) {
    TrapFrame frame = {};
    frame.eax = SYS_WRITE;
    frame.ebx = 99;
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

// count=0 satisfies validate_user_buffer immediately (len==0 short-circuit).
TEST(syscall, write_zero_count) {
    TrapFrame frame = {};
    frame.eax = SYS_WRITE;
    frame.ebx = 1;  // fd = stdout
    frame.ecx = 0;  // buf (irrelevant for len=0)
    frame.edx = 0;  // count = 0
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, 0u);
}

// buf + count wraps past 2^32, so the overflow check rejects it.
TEST(syscall, write_buf_overflow) {
    TrapFrame frame = {};
    frame.eax = SYS_WRITE;
    frame.ebx = 1;            // fd = stdout
    frame.ecx = 0xFFFFFFFEu;  // buf near end of address space
    frame.edx = 4;            // buf + count wraps to 2
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

// buf sits exactly at KERNEL_VMA, so buf+count crosses into kernel space.
TEST(syscall, write_buf_in_kernel_space) {
    TrapFrame frame = {};
    frame.eax = SYS_WRITE;
    frame.ebx = 1;                                  // fd = stdout
    frame.ecx = static_cast<uint32_t>(KERNEL_VMA);  // buf at kernel boundary
    frame.edx = 1;                                  // count = 1
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

// ---------------------------------------------------------------------------
// SYS_READ
// ---------------------------------------------------------------------------

TEST(syscall, read_bad_fd) {
    TrapFrame frame = {};
    frame.eax = SYS_READ;
    frame.ebx = 99;
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

// buf + count wraps past 2^32, so the overflow check rejects it.
TEST(syscall, read_buf_overflow) {
    TrapFrame frame = {};
    frame.eax = SYS_READ;
    frame.ebx = 0;            // fd = stdin
    frame.ecx = 0xFFFFFFFEu;  // buf near end of address space
    frame.edx = 4;            // buf + count wraps to 2
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

// buf sits exactly at KERNEL_VMA, so buf+count crosses into kernel space.
TEST(syscall, read_buf_in_kernel_space) {
    TrapFrame frame = {};
    frame.eax = SYS_READ;
    frame.ebx = 0;                                  // fd = stdin
    frame.ecx = static_cast<uint32_t>(KERNEL_VMA);  // buf at kernel boundary
    frame.edx = 1;                                  // count = 1
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

// ---------------------------------------------------------------------------
// SYS_SBRK
// ---------------------------------------------------------------------------

// The idle process has heap_break == 0, so a zero-increment query returns 0.
TEST(syscall, sbrk_query) {
    TrapFrame frame = {};
    frame.eax = SYS_SBRK;
    frame.ebx = 0;
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, 0u);
}

// An increment that would push the break to exactly KERNEL_VMA is rejected.
// With heap_break=0 and ebx=KERNEL_VMA: new_break = 0 + 0xC0000000 =
// KERNEL_VMA.
TEST(syscall, sbrk_at_kernel_boundary) {
    TrapFrame frame = {};
    frame.eax = SYS_SBRK;
    frame.ebx = static_cast<uint32_t>(KERNEL_VMA);
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

// A negative increment (ebx = -1) from a non-zero break produces a new_break
// below the current break, which sbrk rejects (heap can only grow).
// With heap_break=0x401000 and ebx=0xFFFFFFFF: new_break wraps to 0x400FFF,
// which is less than old_break, triggering the new_break < old_break guard.
TEST(syscall, sbrk_negative_below_break) {
    Process *proc = Scheduler::current();
    vaddr_t orig_break = proc->heap_break;

    proc->heap_break = 0x00401000;

    TrapFrame frame = {};
    frame.eax = SYS_SBRK;
    frame.ebx = 0xFFFFFFFFu;  // -1 as int32_t
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
    ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));

    proc->heap_break = orig_break;
}

// A positive increment on a proper user address space maps new physical pages
// and advances heap_break; the old break is returned.
TEST(syscall, sbrk_allocates_page) {
    auto [pd_phys, pd] = AddressSpace::create();
    ASSERT_NOT_NULL(pd);

    Process *proc = Scheduler::current();
    PageTable *orig_pd = proc->page_directory;
    paddr_t orig_pd_phys = proc->page_directory_phys;
    vaddr_t orig_break = proc->heap_break;

    proc->page_directory = pd;
    proc->page_directory_phys = pd_phys;
    proc->heap_break = 0x00400000;

    TrapFrame frame = {};
    frame.eax = SYS_SBRK;
    frame.ebx = PAGE_SIZE;
    syscall_dispatch(reinterpret_cast<uint32_t>(&frame));

    // Returns old break and advances break by PAGE_SIZE.
    ASSERT_EQ(frame.eax, 0x00400000u);
    ASSERT_EQ(static_cast<uint32_t>(proc->heap_break), 0x00401000u);
    // The page that was newly allocated must be user-writable.
    ASSERT_TRUE(
        AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/true));

    proc->page_directory = orig_pd;
    proc->page_directory_phys = orig_pd_phys;
    proc->heap_break = orig_break;

    AddressSpace::destroy(pd, pd_phys);
}

// ---------------------------------------------------------------------------
// SYS_EXEC
// ---------------------------------------------------------------------------

// A name pointer at or above KERNEL_VMA fails validate_user_buffer immediately.
TEST(syscall, exec_name_in_kernel_space) {
    TrapFrame frame = {};
    frame.eax = SYS_EXEC;
    frame.ebx = static_cast<uint32_t>(KERNEL_VMA);
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
