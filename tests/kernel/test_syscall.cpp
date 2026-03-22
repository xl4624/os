#include <errno.h>
#include <string.h>
#include <sys/syscall.h>

#include "address_space.h"
#include "gdt.h"
#include "ktest.h"
#include "paging.h"
#include "pmm.h"
#include "process.h"
#include "scheduler.h"
#include "syscall.h"
#include "vfs.h"

namespace {

// Stub VFS ops used to register dummy nodes for chdir tests.
int32_t stub_read([[maybe_unused]] VfsNode* node, [[maybe_unused]] std::span<uint8_t> buf,
                  [[maybe_unused]] uint32_t offset) {
  return 0;
}
const VfsOps kStubOps = {.read = stub_read, .write = nullptr, .ioctl = nullptr};

// Maps a fresh physical page at kUserAddr in the CURRENT process's page
// directory (so the existing CR3 sees it), writes `str` into it via the
// kernel virtual alias, and unmap on restore().
struct UserPathPage {
  static constexpr uint32_t kUserAddr = 0x00400000;

  paddr_t page_phys;

  explicit UserPathPage(const char* str) {
    page_phys = kPmm.alloc();
    char* kva = phys_to_virt(page_phys).ptr<char>();
    strncpy(kva, str, PAGE_SIZE - 1);
    kva[PAGE_SIZE - 1] = '\0';

    const Process* proc = Scheduler::current();
    // Map into the currently active page directory so that both
    // validate_user_buffer and direct kernel reads of the user pointer work.
    AddressSpace::map(proc->page_directory, kUserAddr, page_phys,
                      /*writeable=*/true, /*user=*/true);
  }

  static void restore() {
    const Process* proc = Scheduler::current();
    // unmap also frees the physical page back to the PMM.
    AddressSpace::unmap(proc->page_directory, kUserAddr);
  }
};

}  // namespace

// ===========================================================================
// syscall_dispatch
// ===========================================================================

TEST(syscall, invalid_number) {
  TrapFrame frame = {};
  frame.eax = SYS_MAX + 5;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-ENOSYS));
}

// During ktest the current process is the idle process (PID 0).
TEST(syscall, getpid) {
  TrapFrame frame = {};
  frame.eax = SYS_GETPID;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, 0U);
}

// ===========================================================================
// SYS_WRITE
// ===========================================================================

TEST(syscall, write_bad_fd) {
  TrapFrame frame = {};
  frame.eax = SYS_WRITE;
  frame.ebx = 99;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-EBADF));
}

// count=0 satisfies validate_user_buffer immediately (len==0 short-circuit).
TEST(syscall, write_zero_count) {
  TrapFrame frame = {};
  frame.eax = SYS_WRITE;
  frame.ebx = 1;  // fd = stdout
  frame.ecx = 0;  // buf (irrelevant for len=0)
  frame.edx = 0;  // count = 0
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, 0U);
}

// buf + count wraps past 2^32, so the overflow check rejects it.
TEST(syscall, write_buf_overflow) {
  TrapFrame frame = {};
  frame.eax = SYS_WRITE;
  frame.ebx = 1;            // fd = stdout
  frame.ecx = 0xFFFFFFFEU;  // buf near end of address space
  frame.edx = 4;            // buf + count wraps to 2
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-EFAULT));
}

// buf sits exactly at KERNEL_VMA, so buf+count crosses into kernel space.
TEST(syscall, write_buf_in_kernel_space) {
  TrapFrame frame = {};
  frame.eax = SYS_WRITE;
  frame.ebx = 1;                                  // fd = stdout
  frame.ecx = static_cast<uint32_t>(KERNEL_VMA);  // buf at kernel boundary
  frame.edx = 1;                                  // count = 1
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-EFAULT));
}

// ===========================================================================
// SYS_READ
// ===========================================================================

TEST(syscall, read_bad_fd) {
  TrapFrame frame = {};
  frame.eax = SYS_READ;
  frame.ebx = 99;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-EBADF));
}

// buf + count wraps past 2^32, so the overflow check rejects it.
TEST(syscall, read_buf_overflow) {
  TrapFrame frame = {};
  frame.eax = SYS_READ;
  frame.ebx = 0;            // fd = stdin
  frame.ecx = 0xFFFFFFFEU;  // buf near end of address space
  frame.edx = 4;            // buf + count wraps to 2
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-EFAULT));
}

// buf sits exactly at KERNEL_VMA, so buf+count crosses into kernel space.
TEST(syscall, read_buf_in_kernel_space) {
  TrapFrame frame = {};
  frame.eax = SYS_READ;
  frame.ebx = 0;                                  // fd = stdin
  frame.ecx = static_cast<uint32_t>(KERNEL_VMA);  // buf at kernel boundary
  frame.edx = 1;                                  // count = 1
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-EFAULT));
}

// ===========================================================================
// SYS_SBRK
// ===========================================================================

// The idle process has heap_break == 0, so a zero-increment query returns 0.
TEST(syscall, sbrk_query) {
  TrapFrame frame = {};
  frame.eax = SYS_SBRK;
  frame.ebx = 0;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, 0U);
}

// An increment that would push the break to exactly KERNEL_VMA is rejected.
// With heap_break=0 and ebx=KERNEL_VMA: new_break = 0 + 0xC0000000 =
// KERNEL_VMA.
TEST(syscall, sbrk_at_kernel_boundary) {
  TrapFrame frame = {};
  frame.eax = SYS_SBRK;
  frame.ebx = static_cast<uint32_t>(KERNEL_VMA);
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-ENOMEM));
}

// A negative increment (ebx = -1) from a non-zero break produces a new_break
// below the current break, which sbrk rejects (heap can only grow).
// With heap_break=0x401000 and ebx=0xFFFFFFFF: new_break wraps to 0x400FFF,
// which is less than old_break, triggering the new_break < old_break guard.
TEST(syscall, sbrk_negative_below_break) {
  Process* proc = Scheduler::current();
  const vaddr_t orig_break = proc->heap_break;

  proc->heap_break = 0x00401000;

  TrapFrame frame = {};
  frame.eax = SYS_SBRK;
  frame.ebx = 0xFFFFFFFFU;  // -1 as int32_t
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-ENOMEM));

  proc->heap_break = orig_break;
}

// A positive increment on a proper user address space maps new physical pages
// and advances heap_break; the old break is returned.
TEST(syscall, sbrk_allocates_page) {
  auto [pd_phys, pd] = AddressSpace::create();
  ASSERT_NOT_NULL(pd);

  Process* proc = Scheduler::current();
  PageTable* orig_pd = proc->page_directory;
  const paddr_t orig_pd_phys = proc->page_directory_phys;
  const vaddr_t orig_break = proc->heap_break;

  proc->page_directory = pd;
  proc->page_directory_phys = pd_phys;
  proc->heap_break = 0x00400000;

  TrapFrame frame = {};
  frame.eax = SYS_SBRK;
  frame.ebx = PAGE_SIZE;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));

  // Returns old break and advances break by PAGE_SIZE.
  ASSERT_EQ(frame.eax, 0x00400000U);
  ASSERT_EQ(static_cast<uint32_t>(proc->heap_break), 0x00401000U);
  // The page that was newly allocated must be user-writable.
  ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/true));

  proc->page_directory = orig_pd;
  proc->page_directory_phys = orig_pd_phys;
  proc->heap_break = orig_break;

  AddressSpace::destroy(pd, pd_phys);
}

// ===========================================================================
// SYS_EXEC
// ===========================================================================

// A name pointer at or above KERNEL_VMA fails validate_user_buffer immediately.
TEST(syscall, exec_name_in_kernel_space) {
  TrapFrame frame = {};
  frame.eax = SYS_EXEC;
  frame.ebx = static_cast<uint32_t>(KERNEL_VMA);
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-EFAULT));
}

// ===========================================================================
// Scheduler::alloc_user_stack
// ===========================================================================

TEST(syscall, alloc_user_stack_range) {
  auto [pd_phys, pd] = AddressSpace::create();
  ASSERT_NOT_NULL(pd);

  const char* argv[] = {"test"};
  const uint32_t esp = Scheduler::alloc_user_stack(pd, argv);
  ASSERT_NE(esp, 0U);
  ASSERT_TRUE(esp < static_cast<uint32_t>(kUserStackTop));
  ASSERT_TRUE(esp >= static_cast<uint32_t>(kUserStackVA));

  AddressSpace::destroy(pd, pd_phys);
}

TEST(syscall, alloc_user_stack_pages_are_user_mapped) {
  auto [pd_phys, pd] = AddressSpace::create();
  ASSERT_NOT_NULL(pd);

  const char* argv[] = {"hello"};
  const uint32_t esp = Scheduler::alloc_user_stack(pd, argv);
  ASSERT_NE(esp, 0U);
  ASSERT_TRUE(AddressSpace::is_user_mapped(pd, esp, /*writeable=*/true));

  AddressSpace::destroy(pd, pd_phys);
}

// ===========================================================================
// Scheduler::init_trap_frame
// ===========================================================================

TEST(syscall, init_trap_frame_fields) {
  TrapFrame frame = {};
  // Poison GP registers to confirm they are all zeroed by init_trap_frame.
  frame.eax = 0xDEAD;
  frame.ebx = 0xBEEF;

  Scheduler::init_trap_frame(&frame, 0x00401000, 0x00BFFFF0);

  ASSERT_EQ(frame.eip, 0x00401000U);
  ASSERT_EQ(frame.user_esp, 0x00BFFFF0U);
  ASSERT_EQ(frame.eflags, 0x202U);
  ASSERT_EQ(frame.cs, static_cast<uint32_t>(GDT::USER_CODE_SELECTOR));
  ASSERT_EQ(frame.user_ss, static_cast<uint32_t>(GDT::USER_DATA_SELECTOR));
  ASSERT_EQ(frame.ds, static_cast<uint32_t>(GDT::USER_DATA_SELECTOR));
  ASSERT_EQ(frame.eax, 0U);
  ASSERT_EQ(frame.ebx, 0U);
  ASSERT_EQ(frame.ecx, 0U);
  ASSERT_EQ(frame.edx, 0U);
  ASSERT_EQ(frame.esi, 0U);
  ASSERT_EQ(frame.edi, 0U);
  ASSERT_EQ(frame.ebp, 0U);
}

// ===========================================================================
// SYS_CHDIR / SYS_GETCWD
// ===========================================================================

TEST(syscall, chdir_path_in_kernel_space) {
  TrapFrame frame = {};
  frame.eax = SYS_CHDIR;
  frame.ebx = static_cast<uint32_t>(KERNEL_VMA);
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-EFAULT));
}

TEST(syscall, getcwd_returns_current_cwd) {
  Process* proc = Scheduler::current();
  // Put a known CWD in the process.
  strncpy(proc->cwd, "/", sizeof(proc->cwd));

  // Allocate a user-space page to receive the result.
  const UserPathPage up("");

  TrapFrame frame = {};
  frame.eax = SYS_GETCWD;
  frame.ebx = UserPathPage::kUserAddr;
  frame.ecx = PAGE_SIZE;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, 0U);

  // The page is live in CR3; read back via the kernel physical alias.
  const char* kva = phys_to_virt(up.page_phys).ptr<char>();
  ASSERT_STR_EQ(kva, "/");

  UserPathPage::restore();
  strncpy(proc->cwd, "/", sizeof(proc->cwd));
}

TEST(syscall, chdir_absolute_path) {
  Vfs::init();
  ASSERT_NOT_NULL(Vfs::register_node("/bin/sh", VfsNodeType::File, &kStubOps));

  Process* proc = Scheduler::current();
  strncpy(proc->cwd, "/", sizeof(proc->cwd));

  const UserPathPage up("/bin");

  TrapFrame frame = {};
  frame.eax = SYS_CHDIR;
  frame.ebx = UserPathPage::kUserAddr;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, 0U);
  ASSERT_STR_EQ(proc->cwd, "/bin");

  UserPathPage::restore();
  strncpy(proc->cwd, "/", sizeof(proc->cwd));
}

TEST(syscall, chdir_relative_path) {
  Vfs::init();
  ASSERT_NOT_NULL(Vfs::register_node("/bin/sh", VfsNodeType::File, &kStubOps));

  Process* proc = Scheduler::current();
  strncpy(proc->cwd, "/", sizeof(proc->cwd));

  // Relative path "bin" from "/" should resolve to "/bin".
  const UserPathPage up("bin");

  TrapFrame frame = {};
  frame.eax = SYS_CHDIR;
  frame.ebx = UserPathPage::kUserAddr;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, 0U);
  ASSERT_STR_EQ(proc->cwd, "/bin");

  UserPathPage::restore();
  strncpy(proc->cwd, "/", sizeof(proc->cwd));
}

TEST(syscall, chdir_dot_dot) {
  Vfs::init();
  ASSERT_NOT_NULL(Vfs::register_node("/bin/sh", VfsNodeType::File, &kStubOps));

  Process* proc = Scheduler::current();
  strncpy(proc->cwd, "/bin", sizeof(proc->cwd));

  const UserPathPage up("..");

  TrapFrame frame = {};
  frame.eax = SYS_CHDIR;
  frame.ebx = UserPathPage::kUserAddr;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, 0U);
  ASSERT_STR_EQ(proc->cwd, "/");

  UserPathPage::restore();
  strncpy(proc->cwd, "/", sizeof(proc->cwd));
}

TEST(syscall, chdir_dot) {
  Vfs::init();
  ASSERT_NOT_NULL(Vfs::register_node("/bin/sh", VfsNodeType::File, &kStubOps));

  Process* proc = Scheduler::current();
  strncpy(proc->cwd, "/bin", sizeof(proc->cwd));

  const UserPathPage up(".");

  TrapFrame frame = {};
  frame.eax = SYS_CHDIR;
  frame.ebx = UserPathPage::kUserAddr;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, 0U);
  ASSERT_STR_EQ(proc->cwd, "/bin");

  UserPathPage::restore();
  strncpy(proc->cwd, "/", sizeof(proc->cwd));
}

TEST(syscall, chdir_dot_dot_at_root_stays_root) {
  Vfs::init();

  Process* proc = Scheduler::current();
  strncpy(proc->cwd, "/", sizeof(proc->cwd));

  const UserPathPage up("..");

  TrapFrame frame = {};
  frame.eax = SYS_CHDIR;
  frame.ebx = UserPathPage::kUserAddr;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, 0U);
  ASSERT_STR_EQ(proc->cwd, "/");

  UserPathPage::restore();
  strncpy(proc->cwd, "/", sizeof(proc->cwd));
}

TEST(syscall, chdir_nonexistent) {
  Vfs::init();

  Process* proc = Scheduler::current();
  strncpy(proc->cwd, "/", sizeof(proc->cwd));

  const UserPathPage up("/nonexistent");

  TrapFrame frame = {};
  frame.eax = SYS_CHDIR;
  frame.ebx = UserPathPage::kUserAddr;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-ENOENT));
  // CWD must be unchanged on failure.
  ASSERT_STR_EQ(proc->cwd, "/");

  UserPathPage::restore();
}
