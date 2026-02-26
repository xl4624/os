#include "syscall.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "address_space.h"
#include "elf.h"
#include "file.h"
#include "idt.h"
#include "interrupt.h"
#include "keyboard.h"
#include "modules.h"
#include "paging.h"
#include "pipe.h"
#include "pmm.h"
#include "scheduler.h"
#include "shm.h"
#include "tss.h"
#include "tty.h"

static constexpr uint32_t SYSCALL_VECTOR = 0x80;

// ===========================================================================
// Syscall handlers
// ===========================================================================

// Validate that a user pointer range [ptr, ptr+len) is entirely below the
// kernel virtual base and that every page in the range is present and
// user-accessible. writeable additionally requires each PTE to have rw=1.
static bool validate_user_buffer(uint32_t ptr, uint32_t len, bool writeable) {
  if (len == 0) {
    return true;
  }
  // Overflow check and kernel boundary check.
  if (ptr + len <= ptr || ptr + len > KERNEL_VMA) {
    return false;
  }
  const PageTable* pd = Scheduler::current()->page_directory;
  const uint32_t page_mask = PAGE_SIZE - 1;
  for (vaddr_t page = ptr & ~page_mask; page < ptr + len; page += PAGE_SIZE) {
    if (!AddressSpace::is_user_mapped(pd, page, writeable)) {
      return false;
    }
  }
  return true;
}

// SYS_WRITE(fd=ebx, buf=ecx, count=edx)
// Returns number of bytes written, or -1 on error.
static int32_t sys_write(TrapFrame* regs) {
  uint32_t fd_num = regs->ebx;
  uint32_t buf = regs->ecx;
  uint32_t count = regs->edx;

  Process* proc = Scheduler::current();
  if (fd_num >= kMaxFds || !proc->fds[fd_num]) {
    return -1;
  }

  if (!validate_user_buffer(buf, count, /*writeable=*/false)) {
    return -1;
  }

  return file_write(proc->fds[fd_num], reinterpret_cast<uint8_t*>(buf), count);
}

// SYS_READ(fd=ebx, buf=ecx, count=edx)
// Returns number of bytes read, or -1 on error.
static int32_t sys_read(TrapFrame* regs) {
  uint32_t fd_num = regs->ebx;
  uint32_t buf = regs->ecx;
  uint32_t count = regs->edx;

  Process* proc = Scheduler::current();
  if (fd_num >= kMaxFds || !proc->fds[fd_num]) {
    return -1;
  }

  if (!validate_user_buffer(buf, count, /*writeable=*/true)) {
    return -1;
  }

  return file_read(proc->fds[fd_num], reinterpret_cast<uint8_t*>(buf), count);
}

// SYS_EXIT(code=ebx)
static int32_t sys_exit(TrapFrame* regs) {
  uint32_t code = regs->ebx;
  Scheduler::exit_current(code);
  return 0;  // not reached by the exiting process
}

// SYS_SLEEP(ms=ebx)
static int32_t sys_sleep(TrapFrame* regs) {
  uint32_t ms = regs->ebx;
  Scheduler::sleep_current(ms);
  return 0;
}

// SYS_SBRK(increment=ebx)
// Returns old break on success, or (uint32_t)-1 on failure.
static int32_t sys_sbrk(TrapFrame* regs) {
  auto increment = static_cast<int32_t>(regs->ebx);
  Process* proc = Scheduler::current();

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
        for (vaddr_t undo = old_page; undo < va; undo += PAGE_SIZE) {
          AddressSpace::unmap(proc->page_directory, undo);
        }
        return -1;
      }
      AddressSpace::map(proc->page_directory, va, phys,
                        /*writeable=*/true, /*user=*/true);

      // Zero the page so userspace gets clean memory.
      auto* page = phys_to_virt(phys).ptr<uint8_t>();
      memset(page, 0, PAGE_SIZE);
    }
  }

  proc->heap_break = new_break;
  return static_cast<int32_t>(old_break);
}

// SYS_GETPID()
// Returns the PID of the calling process.
static int32_t sys_getpid([[maybe_unused]] TrapFrame* regs) {
  return static_cast<int32_t>(Scheduler::current()->pid);
}

// SYS_EXEC(name=ebx)
// Replaces the current process image with a named module.
// Returns 0 on success, -1 on failure.
static int32_t sys_exec(TrapFrame* regs) {
  uint32_t name_ptr = regs->ebx;

  if (!validate_user_buffer(name_ptr, 1, /*writeable=*/false)) {
    return -1;
  }

  const char* name = reinterpret_cast<const char*>(name_ptr);
  const Module* mod = Modules::find(name);
  if (!mod) {
    return -1;
  }

  Process* proc = Scheduler::current();
  assert(proc != nullptr && "sys_exec(): no current process");

  auto [new_pd_phys, new_pd_virt] = AddressSpace::create();
  if (!new_pd_virt) {
    return -1;
  }

  vaddr_t entry = 0;
  vaddr_t brk = 0;
  if (!Elf::load(mod->data, mod->len, new_pd_virt, entry, brk)) {
    AddressSpace::destroy(new_pd_virt, new_pd_phys);
    return -1;
  }

  uint32_t user_esp = Scheduler::alloc_user_stack(new_pd_virt, name);
  if (user_esp == 0) {
    AddressSpace::destroy(new_pd_virt, new_pd_phys);
    return -1;
  }

  paddr_t old_pd_phys = proc->page_directory_phys;
  PageTable* old_pd = proc->page_directory;

  proc->page_directory_phys = new_pd_phys;
  proc->page_directory = new_pd_virt;
  proc->heap_break = brk;

  AddressSpace::load(new_pd_phys);
  AddressSpace::sync_kernel_mappings(new_pd_virt);
  TSS::set_kernel_stack(reinterpret_cast<uint32_t>(proc->kernel_stack) + kKernelStackSize);

  if (old_pd) {
    // Temporarily switch to the boot page directory to safely free the old
    // one, then reload the new page directory before returning to user mode.
    AddressSpace::load(virt_to_phys(vaddr_t{&boot_page_directory}));
    AddressSpace::destroy(old_pd, old_pd_phys);
    AddressSpace::load(new_pd_phys);
  }

  // Overwrite the current syscall TrapFrame in-place with the new process's
  // initial register state. When syscall_dispatch returns through TRAP_ENTRY,
  // the macro restores from this same frame and irets directly into the new
  // program. schedule() will save this esp as kernel_esp, so context-switch
  // based resume is also correct.
  Scheduler::init_trap_frame(regs, entry, user_esp);

  return 0;
}

// SYS_FORK()
// Returns child PID to parent, 0 to child, or -1 on failure.
static int32_t sys_fork(TrapFrame* regs) {
  return static_cast<int32_t>(Scheduler::fork_current(regs));
}

// SYS_WAITPID(pid=ebx, exit_code_ptr=ecx)
// Blocks until a child exits, returns child PID on success.
// exit_code_ptr (if non-null) receives the child's exit code.
// Returns 0 if blocked (will be restarted by scheduler), or -1 on error.
static int32_t sys_waitpid(TrapFrame* regs) {
  auto pid = static_cast<int32_t>(regs->ebx);
  uint32_t exit_code_ptr = regs->ecx;
  int32_t* exit_code = nullptr;
  if (exit_code_ptr != 0) {
    exit_code = reinterpret_cast<int32_t*>(exit_code_ptr);
  }
  return Scheduler::waitpid_current(pid, exit_code);
}

// SYS_PIPE(pipefd_ptr=ebx)
// Creates a pipe and writes the read/write fd pair into the user-supplied
// int[2] array. Returns 0 on success, -1 on failure.
static int32_t sys_pipe(TrapFrame* regs) {
  uint32_t pipefd_ptr = regs->ebx;

  if (!validate_user_buffer(pipefd_ptr, 2 * sizeof(int32_t), /*writeable=*/true)) {
    return -1;
  }

  Process* proc = Scheduler::current();

  int32_t rfd = fd_alloc(proc->fds.data());
  if (rfd < 0) {
    return -1;
  }

  int32_t wfd = fd_alloc_from(proc->fds.data(), static_cast<uint32_t>(rfd) + 1);
  if (wfd < 0) {
    return -1;
  }

  auto* pipe = new Pipe();
  if (!pipe) {
    return -1;
  }

  auto* rd = new FileDescription{FileType::PipeRead, 1, pipe};
  auto* wr = new FileDescription{FileType::PipeWrite, 1, pipe};
  if (!rd || !wr) {
    delete rd;
    delete wr;
    delete pipe;
    return -1;
  }

  ++pipe->readers;
  ++pipe->writers;

  proc->fds[rfd] = rd;
  proc->fds[wfd] = wr;

  auto* user_fds = reinterpret_cast<int32_t*>(pipefd_ptr);
  user_fds[0] = rfd;
  user_fds[1] = wfd;

  return 0;
}

// SYS_CLOSE(fd=ebx)
// Closes a file descriptor. Returns 0 on success, -1 on error.
static int32_t sys_close(TrapFrame* regs) {
  uint32_t fd_num = regs->ebx;
  Process* proc = Scheduler::current();

  if (fd_num >= kMaxFds || !proc->fds[fd_num]) {
    return -1;
  }

  file_close(proc->fds[fd_num]);
  proc->fds[fd_num] = nullptr;
  return 0;
}

// SYS_DUP2(oldfd=ebx, newfd=ecx)
// Duplicates oldfd onto newfd. Returns newfd on success, -1 on error.
static int32_t sys_dup2(TrapFrame* regs) {
  uint32_t oldfd = regs->ebx;
  uint32_t newfd = regs->ecx;
  Process* proc = Scheduler::current();

  if (oldfd >= kMaxFds || newfd >= kMaxFds || !proc->fds[oldfd]) {
    return -1;
  }

  if (oldfd == newfd) {
    return static_cast<int32_t>(newfd);
  }

  // Close existing description at newfd if open.
  if (proc->fds[newfd]) {
    file_close(proc->fds[newfd]);
  }

  proc->fds[newfd] = proc->fds[oldfd];
  proc->fds[newfd]->ref();
  return static_cast<int32_t>(newfd);
}

// SYS_SHMGET(size=ebx)
// Allocates physical pages for a shared memory region.
// Returns shmid on success, -1 on failure.
static int32_t sys_shmget(TrapFrame* regs) {
  uint32_t size = regs->ebx;
  return Shm::create(size);
}

// SYS_SHMAT(shmid=ebx, vaddr=ecx)
// Maps a shared memory region into the caller's address space.
// Returns 0 on success, -1 on failure.
static int32_t sys_shmat(TrapFrame* regs) {
  uint32_t shmid = regs->ebx;
  vaddr_t vaddr{regs->ecx};
  return Shm::attach(shmid, vaddr);
}

// SYS_SHMDT(vaddr=ebx, size=ecx)
// Unmaps a shared memory region from the caller's address space.
// Returns 0 on success, -1 on failure.
static int32_t sys_shmdt(TrapFrame* regs) {
  vaddr_t vaddr{regs->ebx};
  uint32_t size = regs->ecx;
  return Shm::detach(vaddr, size);
}

// ===========================================================================
// Dispatch table
// ===========================================================================

using syscall_fn = int32_t (*)(TrapFrame*);

static syscall_fn syscall_table[SYS_MAX] = {
    sys_exit,     // 0
    sys_read,     // 1
    sys_write,    // 2
    sys_sleep,    // 3
    sys_sbrk,     // 4
    sys_getpid,   // 5
    sys_exec,     // 6
    sys_fork,     // 7
    sys_waitpid,  // 8
    sys_pipe,     // 9
    sys_close,    // 10
    sys_dup2,     // 11
    sys_shmget,   // 12
    sys_shmat,    // 13
    sys_shmdt,    // 14
};

__BEGIN_DECLS

// Assembly entry point defined in syscall_entry.S.
void syscall_entry();

// Called from syscall_entry.S. Returns the kernel ESP to restore (may be a
// different process's stack after exit/sleep triggers a context switch).
uint32_t syscall_dispatch(uint32_t esp) {
  auto* regs = reinterpret_cast<TrapFrame*>(esp);
  assert(regs != nullptr && "syscall_dispatch(): regs pointer is null");
  uint32_t num = regs->eax;

  if (num >= SYS_MAX) {
    regs->eax = static_cast<uint32_t>(-1);
    return Scheduler::schedule(esp);
  }

  int32_t ret = syscall_table[num](regs);

  if (ret == kSyscallRestart) {
    // Rewind EIP past the "int $0x80" instruction (opcode CD 80, 2 bytes)
    // so it re-executes when the process is rescheduled. EAX still holds
    // the syscall number (we do not overwrite it).
    regs->eip -= 2;
    Scheduler::block_current();
  } else {
    regs->eax = static_cast<uint32_t>(ret);
  }

  // Run the scheduler. If the syscall blocked or exited the current process,
  // this returns a different process's ESP.
  return Scheduler::schedule(esp);
}

__END_DECLS

namespace Syscall {

void init() {
  assert(Interrupt::is_initialized() && "Syscall::init(): Interrupt must be initialized first");
  IDT::set_entry(SYSCALL_VECTOR, reinterpret_cast<uintptr_t>(syscall_entry), IDT::Gate::Interrupt,
                 IDT::Ring::User);
}

}  // namespace Syscall
