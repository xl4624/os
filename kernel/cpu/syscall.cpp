#include "syscall.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unique_ptr.h>

#include "address_space.h"
#include "elf.h"
#include "file.h"
#include "framebuffer.h"
#include "idt.h"
#include "interrupt.h"
#include "paging.h"
#include "pipe.h"
#include "pit.h"
#include "pmm.h"
#include "scheduler.h"
#include "shm.h"
#include "tss.h"
#include "vfs.h"

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
// Returns number of bytes written, or negative errno on error.
static int32_t sys_write(TrapFrame* regs) {
  const uint32_t fd_num = regs->ebx;
  const uint32_t buf = regs->ecx;
  const uint32_t count = regs->edx;

  const Process* proc = Scheduler::current();
  if (fd_num >= kMaxFds || proc->fds[fd_num] == nullptr) {
    return -EBADF;
  }

  if (!validate_user_buffer(buf, count, /*writeable=*/false)) {
    return -EFAULT;
  }

  return file_write(proc->fds[fd_num],
                    std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(buf), count));
}

// SYS_READ(fd=ebx, buf=ecx, count=edx)
// Returns number of bytes read, or negative errno on error.
static int32_t sys_read(TrapFrame* regs) {
  const uint32_t fd_num = regs->ebx;
  const uint32_t buf = regs->ecx;
  const uint32_t count = regs->edx;

  const Process* proc = Scheduler::current();
  if (fd_num >= kMaxFds || proc->fds[fd_num] == nullptr) {
    return -EBADF;
  }

  if (!validate_user_buffer(buf, count, /*writeable=*/true)) {
    return -EFAULT;
  }

  return file_read(proc->fds[fd_num], std::span<uint8_t>(reinterpret_cast<uint8_t*>(buf), count));
}

// SYS_EXIT(code=ebx)
static int32_t sys_exit(TrapFrame* regs) {
  const uint32_t code = regs->ebx;
  Scheduler::exit_current(code);
  return 0;  // not reached by the exiting process
}

// SYS_SLEEP(ms=ebx)
static int32_t sys_sleep(TrapFrame* regs) {
  const uint32_t ms = regs->ebx;
  Scheduler::sleep_current(ms);
  return 0;
}

// SYS_SBRK(increment=ebx)
// Returns old break on success, or (uint32_t)-1 on failure.
static int32_t sys_sbrk(TrapFrame* regs) {
  auto increment = static_cast<int32_t>(regs->ebx);
  Process* proc = Scheduler::current();

  const vaddr_t old_break = proc->heap_break;

  if (increment == 0) {
    return static_cast<int32_t>(old_break);
  }

  const vaddr_t new_break = old_break + static_cast<uint32_t>(increment);

  // Reject if the new break would collide with the user stack, enter kernel
  // space, or wrap around.
  if (new_break >= kUserStackVA || new_break < old_break) {
    return -ENOMEM;
  }

  if (increment > 0) {
    // Map any new pages between old_break and new_break.
    const vaddr_t old_page = (old_break + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    const vaddr_t new_page = (new_break + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    for (vaddr_t va = old_page; va < new_page; va += PAGE_SIZE) {
      const paddr_t phys = kPmm.alloc();
      if (phys == 0) {
        for (vaddr_t undo = old_page; undo < va; undo += PAGE_SIZE) {
          AddressSpace::unmap(proc->page_directory, undo);
        }
        return -ENOMEM;
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

// SYS_OPEN(path=ebx)
// Opens a file by path. Returns fd on success, or negative errno on failure.
// Supports relative paths by prepending the process CWD.
static int32_t sys_open(TrapFrame* regs) {
  const uint32_t path_ptr = regs->ebx;

  if (!validate_user_buffer(path_ptr, 1, /*writeable=*/false)) {
    return -EFAULT;
  }

  const char* upath = reinterpret_cast<const char*>(path_ptr);
  char abs_path[128];

  if (upath[0] != '/') {
    const Process* proc = Scheduler::current();
    const size_t cwd_len = strlen(proc->cwd);
    const size_t path_len = strlen(upath);
    // +2 for potential '/' separator and null terminator
    if (cwd_len + 1 + path_len + 1 > sizeof(abs_path)) {
      return -ENAMETOOLONG;
    }
    memcpy(abs_path, proc->cwd, cwd_len);
    if (abs_path[cwd_len - 1] != '/') {
      abs_path[cwd_len] = '/';
      memcpy(abs_path + cwd_len + 1, upath, path_len + 1);
    } else {
      memcpy(abs_path + cwd_len, upath, path_len + 1);
    }
    upath = abs_path;
  }

  const int32_t fd = Vfs::open(upath);
  return fd >= 0 ? fd : -ENOENT;
}

// SYS_CHDIR(path=ebx)
// Changes the calling process's current working directory.
// Returns 0 on success, or negative errno on error.
static int32_t sys_chdir(TrapFrame* regs) {
  const uint32_t path_ptr = regs->ebx;

  if (!validate_user_buffer(path_ptr, 1, /*writeable=*/false)) {
    return -EFAULT;
  }

  const char* path = reinterpret_cast<const char*>(path_ptr);
  char abs_path[128];

  if (path[0] != '/') {
    const Process* cur = Scheduler::current();
    const size_t cwd_len = strlen(cur->cwd);
    const size_t path_len = strlen(path);
    if (cwd_len + 1 + path_len + 1 > sizeof(abs_path)) {
      return -ENAMETOOLONG;
    }
    memcpy(abs_path, cur->cwd, cwd_len);
    if (abs_path[cwd_len - 1] != '/') {
      abs_path[cwd_len] = '/';
      memcpy(abs_path + cwd_len + 1, path, path_len + 1);
    } else {
      memcpy(abs_path + cwd_len, path, path_len + 1);
    }
    path = abs_path;
  }

  // Strip trailing slash (unless root).
  char norm[128];
  strncpy(norm, path, sizeof(norm) - 1);
  norm[sizeof(norm) - 1] = '\0';
  size_t nlen = strlen(norm);
  if (nlen > 1 && norm[nlen - 1] == '/') {
    norm[nlen - 1] = '\0';
  }

  if (!Vfs::is_directory(norm)) {
    return -ENOENT;
  }

  Process* proc = Scheduler::current();
  strncpy(proc->cwd, norm, sizeof(proc->cwd) - 1);
  proc->cwd[sizeof(proc->cwd) - 1] = '\0';
  return 0;
}

// SYS_GETCWD(buf=ebx, size=ecx)
// Copies the calling process's current working directory into buf.
// Returns 0 on success, or negative errno on error.
static int32_t sys_getcwd(TrapFrame* regs) {
  const uint32_t buf_ptr = regs->ebx;
  const uint32_t size = regs->ecx;

  if (size == 0) {
    return -EINVAL;
  }
  if (!validate_user_buffer(buf_ptr, size, /*writeable=*/true)) {
    return -EFAULT;
  }

  const Process* proc = Scheduler::current();
  const size_t cwd_len = strlen(proc->cwd) + 1;
  if (cwd_len > size) {
    return -ERANGE;
  }
  memcpy(reinterpret_cast<char*>(buf_ptr), proc->cwd, cwd_len);
  return 0;
}

// SYS_GETDENTS(path=ebx, buf=ecx, count=edx)
// Fills buf with up to count dirent entries for the given directory path.
// Returns the number of entries written, or negative errno on error.
static int32_t sys_getdents(TrapFrame* regs) {
  const uint32_t path_ptr = regs->ebx;
  const uint32_t buf_ptr = regs->ecx;
  const uint32_t count = regs->edx;

  if (!validate_user_buffer(path_ptr, 1, /*writeable=*/false)) {
    return -EFAULT;
  }
  if (count == 0) {
    return 0;
  }
  if (!validate_user_buffer(buf_ptr, count * sizeof(dirent), /*writeable=*/true)) {
    return -EFAULT;
  }

  const char* path = reinterpret_cast<const char*>(path_ptr);
  auto* entries = reinterpret_cast<dirent*>(buf_ptr);
  return static_cast<int32_t>(Vfs::getdents(path, entries, count));
}

// SYS_EXEC(path=ebx, argv=ecx)
// Replaces the current process image with a VFS path (e.g. "/bin/sh").
// argv is a NULL-terminated array of string pointers (may be NULL for argc=0).
// Returns 0 on success, -1 on failure.
static int32_t sys_exec(TrapFrame* regs) {
  static constexpr int kMaxExecArgs = 16;
  static constexpr uint32_t kMaxArgLen = 256;

  const uint32_t path_ptr = regs->ebx;
  const uint32_t argv_ptr = regs->ecx;

  if (!validate_user_buffer(path_ptr, 1, /*writeable=*/false)) {
    return -EFAULT;
  }

  const char* path = reinterpret_cast<const char*>(path_ptr);

  // Copy argv strings from the current (old) address space into kernel
  // buffers before we destroy it.
  int argc = 0;
  char arg_bufs[kMaxExecArgs][kMaxArgLen];
  const char* argv_ptrs[kMaxExecArgs];

  if (argv_ptr != 0) {
    if (!validate_user_buffer(argv_ptr, 4, /*writeable=*/false)) {
      return -EFAULT;
    }
    const auto* uargv = reinterpret_cast<const uint32_t*>(argv_ptr);
    while (argc < kMaxExecArgs) {
      // Validate the pointer slot itself.
      if (!validate_user_buffer(argv_ptr + (static_cast<uint32_t>(argc) * 4), 4,
                                /*writeable=*/false)) {
        return -EFAULT;
      }
      const uint32_t str_ptr = uargv[argc];
      if (str_ptr == 0) {
        break;
      }
      if (!validate_user_buffer(str_ptr, 1, /*writeable=*/false)) {
        return -EFAULT;
      }
      const char* src = reinterpret_cast<const char*>(str_ptr);
      strncpy(arg_bufs[argc], src, kMaxArgLen - 1);
      arg_bufs[argc][kMaxArgLen - 1] = '\0';
      argv_ptrs[argc] = arg_bufs[argc];
      ++argc;
    }
  }

  // If no argv was provided, use the path as argv[0].
  if (argc == 0) {
    strncpy(arg_bufs[0], path, kMaxArgLen - 1);
    arg_bufs[0][kMaxArgLen - 1] = '\0';
    argv_ptrs[0] = arg_bufs[0];
    argc = 1;
  }

  const VfsNode* node = Vfs::lookup(path);
  if (node == nullptr || node->data == nullptr) {
    return -ENOENT;
  }

  Process* proc = Scheduler::current();
  assert(proc != nullptr && "sys_exec(): no current process");

  auto [new_pd_phys, new_pd_virt] = AddressSpace::create();
  if (new_pd_virt == nullptr) {
    return -ENOMEM;
  }

  vaddr_t entry = 0;
  vaddr_t brk = 0;
  if (!Elf::load(std::span<const uint8_t>{node->data, node->size}, new_pd_virt, entry, brk)) {
    AddressSpace::destroy(new_pd_virt, new_pd_phys);
    return -ENOEXEC;
  }

  const uint32_t user_esp = Scheduler::alloc_user_stack(
      new_pd_virt, std::span<const char*>{argv_ptrs, static_cast<size_t>(argc)});
  if (user_esp == 0) {
    AddressSpace::destroy(new_pd_virt, new_pd_phys);
    return -ENOMEM;
  }

  const paddr_t old_pd_phys = proc->page_directory_phys;
  PageTable* old_pd = proc->page_directory;

  proc->page_directory_phys = new_pd_phys;
  proc->page_directory = new_pd_virt;
  proc->heap_break = brk;

  AddressSpace::load(new_pd_phys);
  AddressSpace::sync_kernel_mappings(new_pd_virt);
  TSS::set_kernel_stack(reinterpret_cast<uint32_t>(proc->kernel_stack) + kKernelStackSize);

  if (old_pd != nullptr) {
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
// Blocks until a child exits. Returns child PID on success,
// kSyscallRestart if blocked (will be retried), or -ECHILD on error.
static int32_t sys_waitpid(TrapFrame* regs) {
  auto pid = static_cast<int32_t>(regs->ebx);
  const uint32_t exit_code_ptr = regs->ecx;
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
  const uint32_t pipefd_ptr = regs->ebx;

  if (!validate_user_buffer(pipefd_ptr, 2 * sizeof(int32_t), /*writeable=*/true)) {
    return -1;
  }

  Process* proc = Scheduler::current();

  auto rfd = fd_alloc(proc->fds);
  if (!rfd) {
    return -1;
  }

  auto wfd = fd_alloc_from(proc->fds, *rfd + 1);
  if (!wfd) {
    return -1;
  }

  auto pipe = std::make_unique<Pipe>();
  if (!pipe) {
    return -1;
  }

  auto rd = std::make_unique<FileDescription>(
      FileDescription{FileType::PipeRead, 1, pipe.get(), nullptr});
  auto wr = std::make_unique<FileDescription>(
      FileDescription{FileType::PipeWrite, 1, pipe.get(), nullptr});
  if (!rd || !wr) {
    return -1;
  }

  ++pipe->readers;
  ++pipe->writers;

  proc->fds[*rfd] = rd.release();
  proc->fds[*wfd] = wr.release();
  (void)pipe.release();

  auto* user_fds = reinterpret_cast<int32_t*>(pipefd_ptr);
  user_fds[0] = static_cast<int32_t>(*rfd);
  user_fds[1] = static_cast<int32_t>(*wfd);

  return 0;
}

// SYS_CLOSE(fd=ebx)
// Closes a file descriptor. Returns 0 on success, or negative errno on error.
static int32_t sys_close(TrapFrame* regs) {
  const uint32_t fd_num = regs->ebx;
  Process* proc = Scheduler::current();

  if (fd_num >= kMaxFds || proc->fds[fd_num] == nullptr) {
    return -EBADF;
  }

  file_close(proc->fds[fd_num]);
  proc->fds[fd_num] = nullptr;
  return 0;
}

// SYS_DUP2(oldfd=ebx, newfd=ecx)
// Duplicates oldfd onto newfd. Returns newfd on success, or negative errno on error.
static int32_t sys_dup2(TrapFrame* regs) {
  const uint32_t oldfd = regs->ebx;
  const uint32_t newfd = regs->ecx;
  Process* proc = Scheduler::current();

  if (oldfd >= kMaxFds || newfd >= kMaxFds || proc->fds[oldfd] == nullptr) {
    return -EBADF;
  }

  if (oldfd == newfd) {
    return static_cast<int32_t>(newfd);
  }

  // Close existing description at newfd if open.
  if (proc->fds[newfd] != nullptr) {
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
  const uint32_t size = regs->ebx;
  return Shm::create(size);
}

// SYS_SHMAT(shmid=ebx, vaddr=ecx)
// Maps a shared memory region into the caller's address space.
// Returns 0 on success, -1 on failure.
static int32_t sys_shmat(TrapFrame* regs) {
  const uint32_t shmid = regs->ebx;
  const vaddr_t vaddr{regs->ecx};
  return Shm::attach(shmid, vaddr);
}

// SYS_SHMDT(vaddr=ebx, size=ecx)
// Unmaps a shared memory region from the caller's address space.
// Returns 0 on success, -1 on failure.
static int32_t sys_shmdt(TrapFrame* regs) {
  const vaddr_t vaddr{regs->ebx};
  const uint32_t size = regs->ecx;
  return Shm::detach(vaddr, size);
}

// SYS_GETTICKS()
// Returns the number of PIT ticks since boot (10 ms resolution).
// The 64-bit tick count is split: low 32 bits in eax, high 32 bits in edx.
static int32_t sys_getticks(TrapFrame* regs) {
  const uint64_t ticks = PIT::get_ticks();
  regs->edx = static_cast<uint32_t>(ticks >> 32);
  return static_cast<int32_t>(ticks & 0xFFFFFFFF);
}

// SYS_LSEEK(fd=ebx, offset=ecx, whence=edx)
// Sets the file offset for a VFS-backed file descriptor.
// whence: 0=SEEK_SET, 1=SEEK_CUR, 2=SEEK_END
// Returns the new offset on success, -1 on error.
static int32_t sys_lseek(TrapFrame* regs) {
  const uint32_t fd_num = regs->ebx;
  const auto offset = static_cast<int32_t>(regs->ecx);
  const uint32_t whence = regs->edx;

  const Process* proc = Scheduler::current();
  if (fd_num >= kMaxFds || proc->fds[fd_num] == nullptr) {
    return -1;
  }

  const FileDescription* fd = proc->fds[fd_num];
  if (fd->type != FileType::VfsNode || fd->vfs == nullptr) {
    return -1;
  }

  VfsFileDescription* vfs_fd = fd->vfs;
  if (vfs_fd->node == nullptr || vfs_fd->node->type != VfsNodeType::File) {
    return -1;  // only regular files support seeking
  }

  int64_t base = 0;
  switch (whence) {
    case 0:  // SEEK_SET
      base = 0;
      break;
    case 1:  // SEEK_CUR
      base = static_cast<int64_t>(vfs_fd->offset);
      break;
    case 2:  // SEEK_END
      base = static_cast<int64_t>(vfs_fd->node->size);
      break;
    default:
      return -1;
  }

  const int64_t new_offset = base + offset;
  if (new_offset < 0 || new_offset > INT32_MAX) {
    return -1;
  }

  vfs_fd->offset = static_cast<uint32_t>(new_offset);
  return static_cast<int32_t>(new_offset);
}

// SYS_FB_FLIP(buf=ebx, src_w=ecx, src_h=edx)
// Scales and blits a user framebuffer to the hardware display.
// Returns 0 on success, -1 on error.
static int32_t sys_fb_flip(TrapFrame* regs) {
  const uint32_t buf = regs->ebx;
  const uint32_t src_w = regs->ecx;
  const uint32_t src_h = regs->edx;

  if (src_w == 0 || src_h == 0) {
    return -EINVAL;
  }
  if (!Framebuffer::is_available()) {
    return -ENODEV;
  }

  // Guard against overflow before multiplying.
  if (src_w > 4096 || src_h > 4096) {
    return -EINVAL;
  }
  const uint32_t buf_size = src_w * src_h * 4;
  if (!validate_user_buffer(buf, buf_size, /*writeable=*/false)) {
    return -EFAULT;
  }

  Framebuffer::blit_scaled(reinterpret_cast<const uint32_t*>(buf), src_w, src_h);
  return 0;
}

// SYS_KILL(pid=ebx, sig=ecx)
// Sends signal `sig` to the process with the given pid.
// Returns 0 on success, or negative errno on error.
static int32_t sys_kill(TrapFrame* regs) {
  const auto pid = static_cast<int32_t>(regs->ebx);
  const uint32_t sig = regs->ecx;
  if (sig >= 32) {
    return -EINVAL;
  }
  if (pid > 0) {
    Scheduler::send_signal(static_cast<uint32_t>(pid), sig);
  } else if (pid == 0 || pid == -1) {
    Scheduler::broadcast_signal(sig);
  }
  return 0;
}

// SYS_SIGACTION(sig=ebx, handler=ecx, old_handler_ptr=edx)
// Sets the handler for signal `sig`. handler is a user-space VA, kSigDfl (0),
// or kSigIgn (1). If old_handler_ptr is non-null, the previous handler is
// written there. Returns 0 on success, or negative errno on error.
static int32_t sys_sigaction(TrapFrame* regs) {
  const uint32_t sig = regs->ebx;
  const uint32_t new_handler = regs->ecx;
  const uint32_t old_ptr = regs->edx;

  if (sig == 0 || sig >= 32) {
    return -EINVAL;
  }

  Process* proc = Scheduler::current();

  if (old_ptr != 0) {
    if (!validate_user_buffer(old_ptr, sizeof(uint32_t), /*writeable=*/true)) {
      return -EFAULT;
    }
    *reinterpret_cast<uint32_t*>(old_ptr) = proc->signal_handlers[sig];
  }

  proc->signal_handlers[sig] = new_handler;
  return 0;
}

// SYS_SIGRETURN()
// Restores the saved user context from the SignalFrame on the user stack after
// a signal handler returns via the inline trampoline. The trampoline does ret
// (consuming return_addr) before issuing int $0x80, so user_esp on entry
// here points one dword past the start of SignalFrame.
static int32_t sys_sigreturn(TrapFrame* regs) {
  // After handler ret: user_esp = signal_frame_addr + sizeof(return_addr).
  const uint32_t sf_addr = regs->user_esp - sizeof(uint32_t);

  if (!validate_user_buffer(sf_addr, sizeof(SignalFrame), /*writeable=*/false)) {
    Scheduler::exit_current(SIGSEGV);
    return 0;  // not reached for the current process
  }

  const auto* sf = reinterpret_cast<const SignalFrame*>(sf_addr);

  regs->eip = sf->saved_eip;
  regs->eflags = sf->saved_eflags | 0x200U;  // preserve IF
  regs->user_esp = sf->saved_esp;
  regs->ecx = sf->saved_ecx;
  regs->edx = sf->saved_edx;

  // Return saved_eax; syscall_dispatch will write this back to regs->eax.
  return static_cast<int32_t>(sf->saved_eax);
}

// ===========================================================================
// Dispatch table
// ===========================================================================

using syscall_fn = int32_t (*)(TrapFrame*);

static constexpr std::array<syscall_fn, SYS_MAX> syscall_table = {
    sys_exit,       // 0  SYS_EXIT
    sys_fork,       // 1  SYS_FORK
    sys_read,       // 2  SYS_READ
    sys_write,      // 3  SYS_WRITE
    sys_open,       // 4  SYS_OPEN
    sys_close,      // 5  SYS_CLOSE
    sys_waitpid,    // 6  SYS_WAITPID
    sys_exec,       // 7  SYS_EXEC
    sys_lseek,      // 8  SYS_LSEEK
    sys_getpid,     // 9  SYS_GETPID
    sys_pipe,       // 10 SYS_PIPE
    sys_sbrk,       // 11 SYS_SBRK
    sys_dup2,       // 12 SYS_DUP2
    sys_sleep,      // 13 SYS_SLEEP
    sys_shmget,     // 14 SYS_SHMGET
    sys_shmat,      // 15 SYS_SHMAT
    sys_shmdt,      // 16 SYS_SHMDT
    sys_getticks,   // 17 SYS_GETTICKS
    sys_fb_flip,    // 18 SYS_FB_FLIP
    sys_kill,       // 19 SYS_KILL
    sys_sigaction,  // 20 SYS_SIGACTION
    sys_sigreturn,  // 21 SYS_SIGRETURN
    sys_chdir,      // 22 SYS_CHDIR
    sys_getcwd,     // 23 SYS_GETCWD
    sys_getdents,   // 24 SYS_GETDENTS
};

static_assert(syscall_table[SYS_EXIT] == sys_exit);
static_assert(syscall_table[SYS_FORK] == sys_fork);
static_assert(syscall_table[SYS_READ] == sys_read);
static_assert(syscall_table[SYS_WRITE] == sys_write);
static_assert(syscall_table[SYS_OPEN] == sys_open);
static_assert(syscall_table[SYS_CLOSE] == sys_close);
static_assert(syscall_table[SYS_WAITPID] == sys_waitpid);
static_assert(syscall_table[SYS_EXEC] == sys_exec);
static_assert(syscall_table[SYS_LSEEK] == sys_lseek);
static_assert(syscall_table[SYS_GETPID] == sys_getpid);
static_assert(syscall_table[SYS_PIPE] == sys_pipe);
static_assert(syscall_table[SYS_SBRK] == sys_sbrk);
static_assert(syscall_table[SYS_DUP2] == sys_dup2);
static_assert(syscall_table[SYS_SLEEP] == sys_sleep);
static_assert(syscall_table[SYS_SHMGET] == sys_shmget);
static_assert(syscall_table[SYS_SHMAT] == sys_shmat);
static_assert(syscall_table[SYS_SHMDT] == sys_shmdt);
static_assert(syscall_table[SYS_GETTICKS] == sys_getticks);
static_assert(syscall_table[SYS_FB_FLIP] == sys_fb_flip);
static_assert(syscall_table[SYS_KILL] == sys_kill);
static_assert(syscall_table[SYS_SIGACTION] == sys_sigaction);
static_assert(syscall_table[SYS_SIGRETURN] == sys_sigreturn);
static_assert(syscall_table[SYS_CHDIR] == sys_chdir);
static_assert(syscall_table[SYS_GETCWD] == sys_getcwd);
static_assert(syscall_table[SYS_GETDENTS] == sys_getdents);
static_assert(syscall_table.size() == SYS_MAX);

__BEGIN_DECLS

// Assembly entry point defined in syscall_entry.S.
void syscall_entry();

// Assembly entry point defined in trap_entry.S.
void page_fault_entry();

// Called from syscall_entry.S. Returns the kernel ESP to restore (may be a
// different process's stack after exit/sleep triggers a context switch).
uint32_t syscall_dispatch(uint32_t esp) {
  auto* regs = reinterpret_cast<TrapFrame*>(esp);
  assert(regs != nullptr && "syscall_dispatch(): regs pointer is null");
  const uint32_t num = regs->eax;

  if (num >= SYS_MAX) {
    regs->eax = static_cast<uint32_t>(-ENOSYS);
    Scheduler::check_pending_signals(regs);
    return Scheduler::schedule(esp);
  }

  const int32_t ret = syscall_table[num](regs);

  if (ret == kSyscallRestart) {
    // Rewind EIP past the "int $0x80" instruction (opcode CD 80, 2 bytes)
    // so it re-executes when the process is rescheduled. EAX still holds
    // the syscall number (we do not overwrite it).
    regs->eip -= 2;
    Scheduler::block_current();
  } else {
    regs->eax = static_cast<uint32_t>(ret);
  }

  // Check for pending signals before returning to user mode.
  Scheduler::check_pending_signals(regs);

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
  // Override the ISRWrapper<14> installed by Interrupt::init() with a full
  // TRAP_ENTRY stub so we can deliver SIGSEGV and context-switch on page faults.
  IDT::set_entry(14, reinterpret_cast<uintptr_t>(page_fault_entry), IDT::Gate::Interrupt,
                 IDT::Ring::Kernel);
}

}  // namespace Syscall
