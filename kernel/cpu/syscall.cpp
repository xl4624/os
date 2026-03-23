#include "syscall.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
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

// Resolves '.' and '..' components in an absolute path in-place.
// path must start with '/'. size is the buffer capacity.
static void canonicalize_path(char* path, size_t size) {
  assert(path != nullptr && path[0] == '/' &&
         "canonicalize_path(): path must be non-null and absolute");
  assert(size >= 2 && "canonicalize_path(): buffer too small");
  char out[128];
  size_t out_len = 1;
  out[0] = '/';

  const char* p = path;
  if (*p == '/') {
    ++p;
  }

  while (*p != '\0') {
    while (*p == '/') {
      ++p;
    }
    if (*p == '\0') {
      break;
    }

    const char* start = p;
    while (*p != '\0' && *p != '/') {
      ++p;
    }
    const auto comp_len = static_cast<size_t>(p - start);

    if (comp_len == 1 && start[0] == '.') {
      // '.' -- skip
    } else if (comp_len == 2 && start[0] == '.' && start[1] == '.') {
      // '..' -- go up one level
      if (out_len > 1) {
        --out_len;
        while (out_len > 1 && out[out_len - 1] != '/') {
          --out_len;
        }
        if (out_len > 1) {
          --out_len;
        }
      }
    } else {
      // At root (out_len==1) the leading '/' is already present; otherwise
      // insert a '/' separator between components.
      const bool at_root = (out_len == 1);
      const size_t needed = out_len + (at_root ? 0U : 1U) + comp_len + 1U;
      if (needed > size) {
        break;
      }
      if (!at_root) {
        out[out_len++] = '/';
      }
      memcpy(out + out_len, start, comp_len);
      out_len += comp_len;
    }
  }

  out[out_len] = '\0';
  memcpy(path, out, out_len + 1);
}

// Resolve a user path to an absolute canonicalized path.
// Returns 0 on success or negative errno on failure.
static int32_t resolve_abs_path(uint32_t path_ptr, char* abs_path, size_t abs_len) {
  if (!validate_user_buffer(path_ptr, 1, /*writeable=*/false)) {
    return -EFAULT;
  }
  const char* upath = reinterpret_cast<const char*>(path_ptr);
  if (upath[0] != '/') {
    const Process* proc = Scheduler::current();
    const size_t cwd_len = strlen(proc->cwd);
    const size_t path_len = strlen(upath);
    if (cwd_len + 1 + path_len + 1 > abs_len) {
      return -ENAMETOOLONG;
    }
    memcpy(abs_path, proc->cwd, cwd_len);
    if (abs_path[cwd_len - 1] != '/') {
      abs_path[cwd_len] = '/';
      memcpy(abs_path + cwd_len + 1, upath, path_len + 1);
    } else {
      memcpy(abs_path + cwd_len, upath, path_len + 1);
    }
  } else {
    strncpy(abs_path, upath, abs_len - 1);
    abs_path[abs_len - 1] = '\0';
  }
  canonicalize_path(abs_path, abs_len);
  return 0;
}

// SYS_OPEN(path=ebx)
// Opens a file by path. Returns fd on success, or negative errno on failure.
// Supports relative paths by prepending the process CWD.
static int32_t sys_open(TrapFrame* regs) {
  const uint32_t path_ptr = regs->ebx;

  char abs_path[128];
  const int32_t rc = resolve_abs_path(path_ptr, abs_path, sizeof(abs_path));
  if (rc < 0) {
    return rc;
  }

  const auto flags = static_cast<int32_t>(regs->ecx);
  const mode_t mode = (flags & O_CREAT) != 0 ? static_cast<mode_t>(regs->edx) : 0;
  return Vfs::open(abs_path, flags, mode);
}

// SYS_CHDIR(path=ebx)
// Changes the calling process's current working directory.
// Returns 0 on success, or negative errno on error.
static int32_t sys_chdir(TrapFrame* regs) {
  const uint32_t path_ptr = regs->ebx;

  char abs_path[128];
  const int32_t rc = resolve_abs_path(path_ptr, abs_path, sizeof(abs_path));
  if (rc < 0) {
    return rc;
  }

  const size_t len = strlen(abs_path);
  if (len > 1 && abs_path[len - 1] == '/') {
    abs_path[len - 1] = '\0';
  }

  if (!Vfs::is_directory(abs_path)) {
    return -ENOENT;
  }

  Process* proc = Scheduler::current();
  strncpy(proc->cwd, abs_path, sizeof(proc->cwd) - 1);
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

  char abs_path[128];
  const int32_t rc = resolve_abs_path(path_ptr, abs_path, sizeof(abs_path));
  if (rc < 0) {
    return rc;
  }
  if (count == 0) {
    return 0;
  }
  if (!validate_user_buffer(buf_ptr, count * sizeof(dirent), /*writeable=*/true)) {
    return -EFAULT;
  }

  auto* entries = reinterpret_cast<dirent*>(buf_ptr);
  return static_cast<int32_t>(Vfs::getdents(abs_path, entries, count));
}

// SYS_EXEC(path=ebx, argv=ecx, envp=edx)
// Replaces the current process image with a VFS path (e.g. "/bin/sh").
// argv is a NULL-terminated array of string pointers (may be NULL for argc=0).
// envp is a NULL-terminated array of "NAME=VALUE" strings (may be NULL).
// Returns 0 on success, -1 on failure.
static int32_t sys_exec(TrapFrame* regs) {
  static constexpr int kMaxExecArgs = 16;
  static constexpr int kMaxExecEnv = 64;
  static constexpr uint32_t kMaxArgLen = 256;
  static constexpr uint32_t kMaxEnvLen = 256;

  const uint32_t path_ptr = regs->ebx;
  const uint32_t argv_ptr = regs->ecx;
  const uint32_t envp_ptr = regs->edx;

  if (!validate_user_buffer(path_ptr, 1, /*writeable=*/false)) {
    return -EFAULT;
  }

  const char* path = reinterpret_cast<const char*>(path_ptr);

  // TODO: these buffers are static to avoid blowing the 16 KB kernel stack (arg_bufs alone is 4 KB,
  // env_bufs is 16 KB).
  // Ideally this should allocate the new user stack pages first, then copy argv/envp strings
  // directly from the old address space into the new stack pages via phys_to_virt() - no
  // intermediate kernel buffers needed at all.
  int argc = 0;
  static char arg_bufs[kMaxExecArgs][kMaxArgLen];
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

  // Copy envp strings.
  int envc = 0;
  static char env_bufs[kMaxExecEnv][kMaxEnvLen];
  const char* env_ptrs[kMaxExecEnv];

  if (envp_ptr != 0) {
    if (!validate_user_buffer(envp_ptr, 4, /*writeable=*/false)) {
      return -EFAULT;
    }
    const auto* uenvp = reinterpret_cast<const uint32_t*>(envp_ptr);
    while (envc < kMaxExecEnv) {
      if (!validate_user_buffer(envp_ptr + (static_cast<uint32_t>(envc) * 4), 4,
                                /*writeable=*/false)) {
        return -EFAULT;
      }
      const uint32_t str_ptr = uenvp[envc];
      if (str_ptr == 0) {
        break;
      }
      if (!validate_user_buffer(str_ptr, 1, /*writeable=*/false)) {
        return -EFAULT;
      }
      const char* src = reinterpret_cast<const char*>(str_ptr);
      strncpy(env_bufs[envc], src, kMaxEnvLen - 1);
      env_bufs[envc][kMaxEnvLen - 1] = '\0';
      env_ptrs[envc] = env_bufs[envc];
      ++envc;
    }
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
      new_pd_virt, std::span<const char*>{argv_ptrs, static_cast<size_t>(argc)},
      std::span<const char*>{env_ptrs, static_cast<size_t>(envc)});
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

  // Close file descriptors marked FD_CLOEXEC.
  for (uint32_t i = 0; i < kMaxFds; ++i) {
    if (proc->fds[i] != nullptr && (proc->fd_flags[i] & FD_CLOEXEC) != 0) {
      file_close(proc->fds[i]);
      proc->fds[i] = nullptr;
      proc->fd_flags[i] = 0;
    }
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

  auto rd = std::make_unique<FileDescription>(FileDescription{
      .type = FileType::PipeRead, .ref_count = 1, .pipe = pipe.get(), .vfs = nullptr});
  auto wr = std::make_unique<FileDescription>(FileDescription{
      .type = FileType::PipeWrite, .ref_count = 1, .pipe = pipe.get(), .vfs = nullptr});
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
  proc->fd_flags[newfd] = 0;  // dup2 clears FD_CLOEXEC on the new fd
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

// SYS_CLOCK_GETTIME(clk_id=ebx, tp=ecx)
// Fills a userspace timespec with the current monotonic time.
static int32_t sys_clock_gettime(TrapFrame* regs) {
  const auto clk_id = static_cast<int32_t>(regs->ebx);
  const uint32_t tp_ptr = regs->ecx;

  // Only CLOCK_MONOTONIC is supported (same value as in libc time.h).
  if (clk_id != 1) {
    return -EINVAL;
  }

  if (!validate_user_buffer(tp_ptr, 8, /*writeable=*/true)) {
    return -EFAULT;
  }

  const uint64_t ticks = PIT::get_ticks();
  constexpr uint32_t kTicksPerSec = 100;
  constexpr uint32_t kNsPerTick = 10'000'000;  // 10 ms = 10,000,000 ns

  auto* tp = reinterpret_cast<int32_t*>(tp_ptr);
  tp[0] = static_cast<int32_t>(ticks / kTicksPerSec);                 // tv_sec
  tp[1] = static_cast<int32_t>((ticks % kTicksPerSec) * kNsPerTick);  // tv_nsec
  return 0;
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

// SYS_IOCTL(fd=ebx, request=ecx, arg=edx)
// Performs a device-specific control operation on a file descriptor.
// Returns 0 on success, or negative errno on error.
static int32_t sys_ioctl(TrapFrame* regs) {
  const uint32_t fd_num = regs->ebx;
  const uint32_t request = regs->ecx;
  const uint32_t arg_ptr = regs->edx;

  const Process* proc = Scheduler::current();
  if (fd_num >= kMaxFds || proc->fds[fd_num] == nullptr) {
    return -EBADF;
  }

  FileDescription* fd = proc->fds[fd_num];

  void* arg = reinterpret_cast<void*>(arg_ptr);

  // Validate the argument buffer for known requests.
  if (request == TIOCGWINSZ &&
      !validate_user_buffer(arg_ptr, sizeof(struct winsize), /*writeable=*/true)) {
    return -EFAULT;
  }
  if ((request == TCGETS) &&
      !validate_user_buffer(arg_ptr, sizeof(struct termios), /*writeable=*/true)) {
    return -EFAULT;
  }
  if ((request == TCSETS) &&
      !validate_user_buffer(arg_ptr, sizeof(struct termios), /*writeable=*/false)) {
    return -EFAULT;
  }

  // Terminal fds (stdin/stdout/stderr) are not backed by VFS nodes, but
  // they still support tty ioctls (TIOCGWINSZ, TCGETS, TCSETS).
  if (fd->type == FileType::TerminalRead || fd->type == FileType::TerminalWrite) {
    return Vfs::tty_ioctl(request, arg);
  }

  if (fd->type != FileType::VfsNode || fd->vfs == nullptr) {
    return -ENOTTY;
  }

  return Vfs::ioctl(fd, request, arg);
}

// SYS_STAT(path=ebx, buf=ecx)
// Returns 0 on success, or negative errno on failure.
static int32_t sys_stat(TrapFrame* regs) {
  const uint32_t path_ptr = regs->ebx;
  const uint32_t buf_ptr = regs->ecx;

  char abs_path[128];
  const int32_t rc = resolve_abs_path(path_ptr, abs_path, sizeof(abs_path));
  if (rc < 0) {
    return rc;
  }

  if (!validate_user_buffer(buf_ptr, sizeof(struct stat), /*writeable=*/true)) {
    return -EFAULT;
  }
  auto* buf = reinterpret_cast<struct stat*>(buf_ptr);
  return Vfs::stat_path(abs_path, buf);
}

// SYS_FSTAT(fd=ebx, buf=ecx)
// Returns 0 on success, or negative errno on failure.
static int32_t sys_fstat(TrapFrame* regs) {
  const uint32_t fd_num = regs->ebx;
  const uint32_t buf_ptr = regs->ecx;

  const Process* proc = Scheduler::current();
  if (fd_num >= kMaxFds || proc->fds[fd_num] == nullptr) {
    return -EBADF;
  }
  const FileDescription* fd = proc->fds[fd_num];
  if (fd->type != FileType::VfsNode || fd->vfs == nullptr || fd->vfs->node == nullptr) {
    return -EBADF;
  }
  if (!validate_user_buffer(buf_ptr, sizeof(struct stat), /*writeable=*/true)) {
    return -EFAULT;
  }
  auto* buf = reinterpret_cast<struct stat*>(buf_ptr);
  return Vfs::stat_node(fd->vfs->node, buf);
}

// SYS_MKDIR(path=ebx, mode=ecx)
// Returns 0 on success, or negative errno on failure.
static int32_t sys_mkdir(TrapFrame* regs) {
  const uint32_t path_ptr = regs->ebx;

  char abs_path[128];
  const int32_t rc = resolve_abs_path(path_ptr, abs_path, sizeof(abs_path));
  if (rc < 0) {
    return rc;
  }

  return Vfs::fs_mkdir(abs_path);
}

// SYS_UNLINK(path=ebx)
// Returns 0 on success, or negative errno on failure.
static int32_t sys_unlink(TrapFrame* regs) {
  const uint32_t path_ptr = regs->ebx;

  char abs_path[128];
  const int32_t rc = resolve_abs_path(path_ptr, abs_path, sizeof(abs_path));
  if (rc < 0) {
    return rc;
  }

  return Vfs::fs_unlink(abs_path);
}

// SYS_RENAME(old=ebx, new=ecx)
// Returns 0 on success, or negative errno on failure.
static int32_t sys_rename(TrapFrame* regs) {
  const uint32_t old_ptr = regs->ebx;
  const uint32_t new_ptr = regs->ecx;

  char old_path[128];
  const int32_t rc1 = resolve_abs_path(old_ptr, old_path, sizeof(old_path));
  if (rc1 < 0) {
    return rc1;
  }

  char new_path[128];
  const int32_t rc2 = resolve_abs_path(new_ptr, new_path, sizeof(new_path));
  if (rc2 < 0) {
    return rc2;
  }

  return Vfs::fs_rename(old_path, new_path);
}

// SYS_FCNTL(fd=ebx, cmd=ecx, arg=edx)
static int32_t sys_fcntl(TrapFrame* regs) {
  const uint32_t fd = regs->ebx;
  const auto cmd = static_cast<int32_t>(regs->ecx);
  const uint32_t arg = regs->edx;

  Process* proc = Scheduler::current();
  if (fd >= kMaxFds || proc->fds[fd] == nullptr) {
    return -EBADF;
  }

  switch (cmd) {
    case F_DUPFD: {
      auto slot = fd_alloc_from(proc->fds, arg);
      if (!slot) {
        return -EMFILE;
      }
      proc->fds[*slot] = proc->fds[fd];
      proc->fds[*slot]->ref();
      proc->fd_flags[*slot] = 0;
      return static_cast<int32_t>(*slot);
    }
    case F_GETFD:
      return static_cast<int32_t>(proc->fd_flags[fd]);
    case F_SETFD:
      proc->fd_flags[fd] = arg & FD_CLOEXEC;
      return 0;
    case F_GETFL: {
      const FileDescription* desc = proc->fds[fd];
      if (desc->type == FileType::VfsNode && desc->vfs != nullptr) {
        return desc->vfs->open_flags;
      }
      if (desc->type == FileType::PipeRead) {
        return O_RDONLY;
      }
      if (desc->type == FileType::PipeWrite) {
        return O_WRONLY;
      }
      return O_RDWR;
    }
    case F_SETFL: {
      // Only O_APPEND and O_NONBLOCK are modifiable via F_SETFL.
      constexpr int32_t kModifiable = O_APPEND | O_NONBLOCK;
      const FileDescription* desc = proc->fds[fd];
      if (desc->type == FileType::VfsNode && desc->vfs != nullptr) {
        desc->vfs->open_flags =
            (desc->vfs->open_flags & ~kModifiable) | (static_cast<int32_t>(arg) & kModifiable);
        return 0;
      }
      return -EINVAL;
    }
    default:
      return -EINVAL;
  }
}

// SYS_MOUNT(target=ebx, fstype=ecx) -- stub
// TODO: implement; the actual mount currently happens at boot via Fat::init_vfs().
static int32_t sys_mount([[maybe_unused]] TrapFrame* regs) { return -ENOSYS; }

// SYS_UMOUNT(target=ebx)
static int32_t sys_umount(TrapFrame* regs) {
  const uint32_t target_ptr = regs->ebx;

  char target[128];
  const int32_t rc = resolve_abs_path(target_ptr, target, sizeof(target));
  if (rc < 0) {
    return rc;
  }

  return Vfs::unmount(target);
}

// ===========================================================================
// Dispatch table
// ===========================================================================

using syscall_fn = int32_t (*)(TrapFrame*);

static constexpr std::array<syscall_fn, SYS_MAX> syscall_table = {
    sys_exit,           // 0  SYS_EXIT
    sys_fork,           // 1  SYS_FORK
    sys_read,           // 2  SYS_READ
    sys_write,          // 3  SYS_WRITE
    sys_open,           // 4  SYS_OPEN
    sys_close,          // 5  SYS_CLOSE
    sys_waitpid,        // 6  SYS_WAITPID
    sys_exec,           // 7  SYS_EXEC
    sys_lseek,          // 8  SYS_LSEEK
    sys_getpid,         // 9  SYS_GETPID
    sys_pipe,           // 10 SYS_PIPE
    sys_sbrk,           // 11 SYS_SBRK
    sys_dup2,           // 12 SYS_DUP2
    sys_clock_gettime,  // 13 SYS_CLOCK_GETTIME
    sys_sleep,          // 14 SYS_SLEEP
    sys_shmget,         // 15 SYS_SHMGET
    sys_shmat,          // 16 SYS_SHMAT
    sys_shmdt,          // 17 SYS_SHMDT
    sys_fb_flip,        // 18 SYS_FB_FLIP
    sys_kill,           // 19 SYS_KILL
    sys_sigaction,      // 20 SYS_SIGACTION
    sys_sigreturn,      // 21 SYS_SIGRETURN
    sys_chdir,          // 22 SYS_CHDIR
    sys_getcwd,         // 23 SYS_GETCWD
    sys_getdents,       // 24 SYS_GETDENTS
    sys_ioctl,          // 25 SYS_IOCTL
    sys_stat,           // 26 SYS_STAT
    sys_fstat,          // 27 SYS_FSTAT
    sys_mkdir,          // 28 SYS_MKDIR
    sys_unlink,         // 29 SYS_UNLINK
    sys_rename,         // 30 SYS_RENAME
    sys_fcntl,          // 31 SYS_FCNTL
    sys_mount,          // 32 SYS_MOUNT
    sys_umount,         // 33 SYS_UMOUNT
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
static_assert(syscall_table[SYS_CLOCK_GETTIME] == sys_clock_gettime);
static_assert(syscall_table[SYS_SLEEP] == sys_sleep);
static_assert(syscall_table[SYS_SHMGET] == sys_shmget);
static_assert(syscall_table[SYS_SHMAT] == sys_shmat);
static_assert(syscall_table[SYS_SHMDT] == sys_shmdt);
static_assert(syscall_table[SYS_FB_FLIP] == sys_fb_flip);
static_assert(syscall_table[SYS_KILL] == sys_kill);
static_assert(syscall_table[SYS_SIGACTION] == sys_sigaction);
static_assert(syscall_table[SYS_SIGRETURN] == sys_sigreturn);
static_assert(syscall_table[SYS_CHDIR] == sys_chdir);
static_assert(syscall_table[SYS_GETCWD] == sys_getcwd);
static_assert(syscall_table[SYS_GETDENTS] == sys_getdents);
static_assert(syscall_table[SYS_IOCTL] == sys_ioctl);
static_assert(syscall_table[SYS_STAT] == sys_stat);
static_assert(syscall_table[SYS_FSTAT] == sys_fstat);
static_assert(syscall_table[SYS_MKDIR] == sys_mkdir);
static_assert(syscall_table[SYS_UNLINK] == sys_unlink);
static_assert(syscall_table[SYS_RENAME] == sys_rename);
static_assert(syscall_table[SYS_FCNTL] == sys_fcntl);
static_assert(syscall_table[SYS_MOUNT] == sys_mount);
static_assert(syscall_table[SYS_UMOUNT] == sys_umount);
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
