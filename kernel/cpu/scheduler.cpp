#include "scheduler.h"

#include <array.h>
#include <assert.h>
#include <cstdint.h>
#include <errno.h>
#include <signal.h>
#include <span.h>
#include <stdio.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/syscall.h>
#include <utility.h>

#include "address_space.h"
#include "elf.h"
#include "file.h"
#include "gdt.h"
#include "heap.h"
#include "idt.h"
#include "interrupt.h"
#include "paging.h"
#include "pic.h"
#include "pit.h"
#include "pmm.h"
#include "process.h"
#include "shm.h"
#include "tss.h"
#include "vfs.h"

__BEGIN_DECLS

// Assembly entry point defined in timer_entry.S.
void timer_entry();

// Kernel stack symbol from arch/boot.S.
extern char stack_top[];

uint32_t timer_dispatch(uint32_t esp) {
  PIT::tick();
  PIC::send_eoi(0);
  auto* regs = reinterpret_cast<TrapFrame*>(esp);
  Scheduler::check_pending_signals(regs);
  return Scheduler::schedule(esp);
}

// Page fault dispatch. Called from page_fault_entry in trap_entry.S after the
// CPU's error code has been discarded. Delivers SIGSEGV for user-mode faults;
// panics for kernel-mode faults.
uint32_t page_fault_dispatch(uint32_t esp) {
  auto* regs = reinterpret_cast<TrapFrame*>(esp);
  if ((regs->cs & 3) == 3) {
    // User-mode page fault: deliver SIGSEGV.
    Scheduler::send_signal(Scheduler::current()->pid, SIGSEGV);
    Scheduler::check_pending_signals(regs);
  } else {
    uint32_t fault_addr = 0;  // NOLINT(misc-const-correctness)
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));
    printf("Kernel page fault at 0x%08x (eip=0x%08x)\n", fault_addr,
           static_cast<unsigned>(regs->eip));
    halt_and_catch_fire();
  }
  return Scheduler::schedule(esp);
}

__END_DECLS

// =============================================================================
// Process Management
// =============================================================================
//
// This module handles everything related to running user processes:
// - Creating and destroying processes
// - Deciding which process runs next (scheduling)
// - Switching between processes (context switching)
// - Putting processes to sleep and waking them up
//
// When the system boots, we create a special "idle" process that does nothing
// but wait. This ensures the CPU always has something to do when no user
// processes are ready to run.

namespace {

std::array<Process, kMaxProcesses> process_table;
uint32_t next_pid = 0;

Process* current_process = nullptr;
Process* ready_head = nullptr;
Process* ready_tail = nullptr;
Process* blocked_head = nullptr;
Process* idle_process = nullptr;

bool initialized = false;
bool started = false;

Process* alloc_process() {
  if (next_pid >= kMaxProcesses) {
    return nullptr;
  }
  Process* p = &process_table[next_pid];
  memset(p, 0, sizeof(Process));
  p->pid = next_pid++;
  p->state = ProcessState::Ready;
  p->cwd[0] = '/';
  p->cwd[1] = '\0';
  return p;
}

void enqueue_ready(Process* p) {
  assert(p != nullptr && "enqueue_ready(): null process");
  p->next = nullptr;
  p->state = ProcessState::Ready;
  if (ready_tail == nullptr) {
    ready_head = ready_tail = p;
  } else {
    ready_tail->next = p;
    ready_tail = p;
  }
}

Process* dequeue_ready() {
  if (ready_head == nullptr) {
    return nullptr;
  }
  Process* p = ready_head;
  ready_head = ready_head->next;
  if (ready_head == nullptr) {
    ready_tail = nullptr;
  }
  p->next = nullptr;
  return p;
}

void enqueue_blocked(Process* p) {
  p->state = ProcessState::Blocked;
  p->next = blocked_head;
  blocked_head = p;
}

void wake_sleepers() {
  const uint64_t now = PIT::get_ticks();
  Process* prev = nullptr;
  Process* p = blocked_head;

  while (p != nullptr) {
    Process* next = p->next;
    if (now >= p->wake_tick) {
      if (prev != nullptr) {
        prev->next = next;
      } else {
        blocked_head = next;
      }
      // Only re-queue if still blocked; processes killed while blocked will be Zombie.
      if (p->state == ProcessState::Blocked) {
        enqueue_ready(p);
      }
    } else {
      prev = p;
    }
    p = next;
  }
}

// Switches from the current process to the target process. This updates
// the TSS (so system calls use the right kernel stack), loads the new
// address space, and returns the new process's kernel stack pointer.
uint32_t switch_to(Process* target) {
  assert(target != nullptr && "switch_to(): null process");

  current_process = target;
  target->state = ProcessState::Running;

  // Update TSS.esp0 so ring-3 interrupts land on this process's kernel stack.
  TSS::set_kernel_stack(reinterpret_cast<uint32_t>(target->kernel_stack) + kKernelStackSize);

  AddressSpace::sync_kernel_mappings(target->page_directory);
  AddressSpace::load(target->page_directory_phys);

  return target->kernel_esp;
}

}  // namespace

namespace Scheduler {

void init() {
  assert(!initialized && "Scheduler::init(): called more than once");

  // Create the idle process. This process runs when no other process is ready to run.
  idle_process = alloc_process();
  assert(idle_process && "Scheduler::init(): failed to allocate idle process");
  idle_process->state = ProcessState::Running;
  idle_process->page_directory = &boot_page_directory;
  idle_process->page_directory_phys = virt_to_phys(vaddr_t{&boot_page_directory});
  idle_process->kernel_stack =
      reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(stack_top) - kKernelStackSize);
  idle_process->kernel_esp = 0;  // will be set on first schedule()
  fd_init_stdio(idle_process->fds);

  current_process = idle_process;
  initialized = true;

  // Install the timer interrupt handler. Scheduling is gated by start().
  IDT::set_entry(32, reinterpret_cast<uintptr_t>(timer_entry), IDT::Gate::Interrupt,
                 IDT::Ring::Kernel);
  PIC::unmask(0);
}

void start() {
  assert(initialized && "Scheduler::start(): must call init() first");
  started = true;
}

bool is_initialized() { return initialized; }

uint32_t schedule(uint32_t esp) {
  // start() hasn't been called yet, so there's nothing to schedule.
  // Keep running on the initial kernel stack.
  if (!started) {
    return esp;
  }

  // Save current process's kernel ESP so we can resume it later.
  current_process->kernel_esp = esp;

  // Check if any sleeping processes have waited long enough.
  wake_sleepers();

  Process* next = dequeue_ready();
  if (next == nullptr) {
    // Nothing ready - stay on current if it's still running, or idle.
    if (current_process->state == ProcessState::Running) {
      return esp;
    }
    // Current was blocked/zombie, switch to idle.
    return switch_to(idle_process);
  }

  // If current is still running (preempted), put it back in the ready
  // queue so it can run again later.
  if (current_process->state == ProcessState::Running && current_process != idle_process) {
    enqueue_ready(current_process);
  }

  // If it's the same process? We can just keep running.
  if (next == current_process) {
    next->state = ProcessState::Running;
    return esp;
  }

  return switch_to(next);
}

void exit_current(uint32_t exit_code) {
  assert(current_process != idle_process && "exit_current(): cannot exit idle process");

  printf("Process %u exited with code %u\n", current_process->pid, exit_code);

  // Close all file descriptors before destroying the address space.
  for (auto& fd : current_process->fds) {
    if (fd != nullptr) {
      file_close(fd);
      fd = nullptr;
    }
  }

  // Detach all shared memory regions so AddressSpace::destroy() does
  // not free the shared physical pages.
  Shm::detach_all(current_process);

  current_process->state = ProcessState::Zombie;
  current_process->exit_code = static_cast<int32_t>(exit_code);

  // Free address space. Must switch to boot page directory first since
  // we cannot free the currently loaded page directory.
  if (current_process != idle_process) {
    AddressSpace::load(virt_to_phys(vaddr_t{&boot_page_directory}));
    AddressSpace::destroy(current_process->page_directory, current_process->page_directory_phys);
    current_process->page_directory = nullptr;
    current_process->page_directory_phys = 0;
  }
}

void sleep_current(uint32_t ms) {
  assert(current_process != idle_process && "sleep_current(): cannot sleep idle process");

  current_process->wake_tick = PIT::get_ticks() + ((static_cast<uint64_t>(ms) + 9) / 10);
  enqueue_blocked(current_process);
  // The schedule() call will switch away from us.
}

void block_current() {
  assert(current_process != idle_process && "block_current(): cannot block idle process");

  // TODO: This creates a 100 Hz polling loop (block, wake, retry syscall,
  // block again). A proper wait queue on the pipe/resource would be more
  // efficient, waking the process only when data is available.
  current_process->wake_tick = 0;
  enqueue_blocked(current_process);
}

uint32_t alloc_user_stack(PageTable* pd, std::span<const char*> argv) {
  paddr_t stack_pages[kUserStackPages];
  for (uint32_t i = 0; i < kUserStackPages; ++i) {
    const paddr_t phys = kPmm.alloc();
    if (phys == 0) {
      for (uint32_t j = 0; j < i; ++j) {
        AddressSpace::unmap(pd, kUserStackVA + j * PAGE_SIZE);
      }
      return 0;
    }
    stack_pages[i] = phys;
    AddressSpace::map(pd, kUserStackVA + i * PAGE_SIZE, phys,
                      /*writeable=*/true, /*user=*/true);
  }

  // Translate a user virtual address into a kernel pointer via the
  // physical pages we just allocated.
  auto kptr = [&](uint32_t uva) -> uint8_t* {
    const uint32_t idx = (uva - kUserStackVA) / PAGE_SIZE;
    const uint32_t off = (uva - kUserStackVA) % PAGE_SIZE;
    return phys_to_virt(stack_pages[idx]).ptr<uint8_t>() + off;
  };

  uint32_t user_esp = kUserStackTop;

  // Push all argv strings and record their user VAs.
  static constexpr size_t kMaxExecArgs = 16;
  assert(argv.size() <= kMaxExecArgs && "alloc_user_stack(): too many arguments");
  uint32_t str_uvas[kMaxExecArgs];

  for (size_t i = argv.size(); i-- > 0;) {
    const size_t len = strlen(argv[i]);
    user_esp -= static_cast<uint32_t>(len + 1);
    memcpy(kptr(user_esp), argv[i], len + 1);
    str_uvas[i] = user_esp;
  }

  // Align esp down to a 4-byte boundary.
  user_esp &= ~3U;

  // Push argv[argc] = NULL.
  user_esp -= 4;
  *reinterpret_cast<uint32_t*>(kptr(user_esp)) = 0;

  // Push argv[argc-1] .. argv[0] pointers.
  for (size_t i = argv.size(); i-- > 0;) {
    user_esp -= 4;
    *reinterpret_cast<uint32_t*>(kptr(user_esp)) = str_uvas[i];
  }

  // Push argc.
  user_esp -= 4;
  *reinterpret_cast<uint32_t*>(kptr(user_esp)) = static_cast<uint32_t>(argv.size());

  return user_esp;
}

void init_trap_frame(TrapFrame* frame, vaddr_t entry, uint32_t user_esp) {
  frame->eip = entry;
  frame->cs = GDT::USER_CODE_SELECTOR;
  frame->eflags = 0x202;  // IF set, reserved bit 1 set
  frame->user_esp = user_esp;
  frame->user_ss = GDT::USER_DATA_SELECTOR;
  frame->ds = GDT::USER_DATA_SELECTOR;
  frame->es = GDT::USER_DATA_SELECTOR;
  frame->fs = GDT::USER_DATA_SELECTOR;
  frame->gs = GDT::USER_DATA_SELECTOR;
  frame->eax = 0;
  frame->ebx = 0;
  frame->ecx = 0;
  frame->edx = 0;
  frame->esi = 0;
  frame->edi = 0;
  frame->ebp = 0;
  frame->esp_dummy = 0;
}

Process* create_process(std::span<const uint8_t> elf_data, const char* name) {
  assert(initialized && "Scheduler::create_process(): scheduler not initialized");

  if (name == nullptr) {
    name = "";
  }

  Process* p = alloc_process();
  if (p == nullptr) {
    printf("Scheduler: failed to allocate process\n");
    return nullptr;
  }

  fd_init_stdio(p->fds);

  auto [pd_phys, pd_virt] = AddressSpace::create();
  p->page_directory_phys = pd_phys;
  p->page_directory = pd_virt;

  vaddr_t entry = 0;
  vaddr_t brk = 0;
  if (!Elf::load(elf_data, pd_virt, entry, brk)) {
    printf("Scheduler: failed to load ELF for process %u\n", p->pid);
    AddressSpace::destroy(pd_virt, pd_phys);
    p->state = ProcessState::Zombie;
    return nullptr;
  }
  p->heap_break = brk;

  const char* argv[] = {name};
  const uint32_t user_esp = alloc_user_stack(pd_virt, argv);
  assert(user_esp != 0 && "Scheduler::create_process(): out of physical memory for user stack");

  p->kernel_stack = reinterpret_cast<uint8_t*>(kmalloc(kKernelStackSize));
  assert(p->kernel_stack && "Scheduler::create_process(): failed to allocate kernel stack");
  memset(p->kernel_stack, 0, kKernelStackSize);

  // Set up initial TrapFrame for iret into user mode.
  auto* kstack_top = reinterpret_cast<uint32_t*>(p->kernel_stack + kKernelStackSize);
  auto* frame =
      reinterpret_cast<TrapFrame*>(reinterpret_cast<uintptr_t>(kstack_top) - sizeof(TrapFrame));

  init_trap_frame(frame, entry, user_esp);

  p->kernel_esp = reinterpret_cast<uint32_t>(frame);

  enqueue_ready(p);

  printf("Scheduler: created process %u from ELF (entry=0x%08x)\n", p->pid,
         static_cast<unsigned>(entry));
  return p;
}

uint32_t fork_current(const TrapFrame* parent_regs) {
  assert(current_process != idle_process && "fork_current(): cannot fork idle process");

  Process* child = alloc_process();
  if (child == nullptr) {
    return static_cast<uint32_t>(-1);
  }

  auto [child_pd_phys, child_pd] = AddressSpace::copy(current_process->page_directory);
  child->page_directory_phys = child_pd_phys;
  child->page_directory = child_pd;
  child->heap_break = current_process->heap_break;
  child->parent_pid = current_process->pid;

  // Inherit the parent's file descriptor table and per-fd flags.
  child->fds = current_process->fds;
  child->fd_flags = current_process->fd_flags;
  for (auto* fd : child->fds) {
    if (fd != nullptr) {
      fd->ref();
    }
  }

  // Re-map shared memory in the child. AddressSpace::copy() deep-copied all user pages,
  // but shared memory pages should reference the same physical frames.
  // Unmap the spurious copies and re-map the originals.
  for (uint32_t i = 0; i < current_process->shm_mapping_count; ++i) {
    const ShmMapping& m = current_process->shm_mappings[i];
    ShmRegion* region = Shm::find_region(m.shm_id);
    if (region == nullptr) {
      continue;
    }

    for (uint32_t p = 0; p < m.num_pages; ++p) {
      AddressSpace::unmap(child_pd, m.vaddr + p * PAGE_SIZE);
      AddressSpace::map(child_pd, m.vaddr + p * PAGE_SIZE, region->pages[p],
                        /*writeable=*/true, /*user=*/true);
    }

    child->shm_mappings[child->shm_mapping_count++] = m;
    ++region->ref_count;
  }

  // Inherit signal handlers; child starts with no pending signals.
  memcpy(child->signal_handlers, current_process->signal_handlers, sizeof(child->signal_handlers));
  child->pending_signals = 0;

  // Inherit working directory and credentials.
  memcpy(child->cwd, current_process->cwd, sizeof(child->cwd));
  child->uid = current_process->uid;
  child->gid = current_process->gid;

  child->kernel_stack = reinterpret_cast<uint8_t*>(kmalloc(kKernelStackSize));
  assert(child->kernel_stack && "fork_current(): failed to allocate kernel stack");
  memset(child->kernel_stack, 0, kKernelStackSize);

  // Place a copy of the parent's TrapFrame at the top of the child's kernel stack.
  // The child returns 0 from fork().
  auto* kstack_top = reinterpret_cast<uint32_t*>(child->kernel_stack + kKernelStackSize);
  auto* child_frame =
      reinterpret_cast<TrapFrame*>(reinterpret_cast<uintptr_t>(kstack_top) - sizeof(TrapFrame));

  *child_frame = *parent_regs;
  child_frame->eax = 0;

  child->kernel_esp = reinterpret_cast<uint32_t>(child_frame);

  enqueue_ready(child);

  printf("Scheduler: forked process %u -> child %u\n", current_process->pid, child->pid);
  return child->pid;
}

int32_t waitpid_current(int32_t pid, int32_t* exit_code_ptr) {
  assert(current_process != idle_process && "waitpid_current(): cannot wait on idle process");

  bool found_child = false;

  for (uint32_t i = 1; i < next_pid; ++i) {
    Process& child = process_table[i];
    if (child.parent_pid != current_process->pid) {
      continue;
    }
    // When waiting for a specific pid, skip non-matching children.
    if (pid > 0 && std::cmp_not_equal(child.pid, pid)) {
      continue;
    }
    if (child.state == ProcessState::Zombie) {
      if (exit_code_ptr != nullptr) {
        *exit_code_ptr = child.exit_code;
      }
      const uint32_t child_pid = child.pid;
      // Free the kernel stack before clearing the slot.
      if (child.kernel_stack != nullptr) {
        kfree(child.kernel_stack);
      }
      child = Process{};
      return static_cast<int32_t>(child_pid);
    }
    // Child exists but is not a zombie yet.
    found_child = true;
  }

  if (!found_child) {
    return -ECHILD;
  }

  // Child exists but hasn't exited; signal syscall_dispatch to block and retry.
  return kSyscallRestart;
}

Process* current() { return current_process; }

void send_signal(uint32_t pid, uint32_t signum) {
  if (signum == 0 || signum >= 32) {
    return;
  }
  for (uint32_t i = 0; i < next_pid; ++i) {
    Process& p = process_table[i];
    if (p.pid != pid || p.state == ProcessState::Zombie) {
      continue;
    }
    p.pending_signals |= (1U << signum);
    // Wake a blocked process so it can have the signal delivered.
    if (p.state == ProcessState::Blocked) {
      Process* prev = nullptr;
      Process* cur = blocked_head;
      while (cur != nullptr) {
        if (cur == &p) {
          if (prev != nullptr) {
            prev->next = cur->next;
          } else {
            blocked_head = cur->next;
          }
          enqueue_ready(cur);
          break;
        }
        prev = cur;
        cur = cur->next;
      }
    }
    return;
  }
}

void broadcast_signal(uint32_t signum) {
  for (uint32_t i = 1; i < next_pid; ++i) {  // skip idle (pid 0)
    const Process& p = process_table[i];
    if (p.state != ProcessState::Zombie && p.state != ProcessState::Empty) {
      send_signal(p.pid, signum);  // NOLINT(readability-magic-numbers)
    }
  }
}

bool has_user_processes() {
  for (uint32_t i = 1; i < next_pid; ++i) {  // skip idle (pid 0)
    const auto s = process_table[i].state;
    if (s != ProcessState::Zombie && s != ProcessState::Empty) {
      return true;
    }
  }
  return false;
}

// Deliver a signal to a user-installed handler by building a SignalFrame on
// the user stack and redirecting execution to the handler.
static void deliver_to_user_handler(TrapFrame* frame, uint32_t sig, uint32_t handler_va) {
  // Trampoline: mov $SYS_SIGRETURN, %eax (5 bytes) + int $0x80 (2 bytes).
  static constexpr uint32_t kTrampolineSize = 7;
  static constexpr uint8_t kTrampoline[kTrampolineSize] = {
      0xB8, static_cast<uint8_t>(SYS_SIGRETURN), 0x00, 0x00,
      0x00,  // mov $SYS_SIGRETURN, %eax
      0xCD,
      0x80,  // int $0x80
  };

  const uint32_t trampoline_addr = frame->user_esp - kTrampolineSize;
  const uint32_t signal_frame_addr = trampoline_addr - sizeof(SignalFrame);

  // Validate that the new stack region is user-writable.
  if (signal_frame_addr >= KERNEL_VMA ||
      signal_frame_addr + kTrampolineSize + sizeof(SignalFrame) > KERNEL_VMA) {
    exit_current(sig);
    return;
  }

  // Write the trampoline and signal frame directly through user virtual
  // addresses (the current page directory is the process's own).
  memcpy(reinterpret_cast<void*>(trampoline_addr), kTrampoline, kTrampolineSize);

  auto* sf = reinterpret_cast<SignalFrame*>(signal_frame_addr);
  sf->return_addr = trampoline_addr;
  sf->signum = sig;
  sf->saved_eax = frame->eax;
  sf->saved_ecx = frame->ecx;
  sf->saved_edx = frame->edx;
  sf->saved_eip = frame->eip;
  sf->saved_eflags = frame->eflags;
  sf->saved_esp = frame->user_esp;

  // Redirect the iret frame to the signal handler.
  frame->eip = handler_va;
  frame->user_esp = signal_frame_addr;
}

void check_pending_signals(TrapFrame* frame) {
  Process* proc = current_process;
  if (proc == idle_process || proc->pending_signals == 0) {
    return;
  }
  // Only deliver signals when returning to user mode.
  if ((frame->cs & 3) != 3) {
    return;
  }

  for (uint32_t sig = 1; sig < 32; ++sig) {
    if ((proc->pending_signals & (1U << sig)) == 0U) {
      continue;
    }
    proc->pending_signals &= ~(1U << sig);

    const uint32_t handler = proc->signal_handlers[sig];
    if (handler == kSigIgn) {
      continue;
    }
    if (handler == kSigDfl) {
      // Default action: terminate the process.
      exit_current(sig);
      return;
    }
    // User-installed handler: build a signal frame and redirect to it.
    deliver_to_user_handler(frame, sig, handler);
    return;  // deliver one signal at a time
  }
}

bool is_vfs_node_open(const VfsNode* node) {
  for (uint32_t i = 0; i < kMaxProcesses; ++i) {
    if (process_table[i].state == ProcessState::Empty) {
      continue;
    }
    for (uint32_t j = 0; j < kMaxFds; ++j) {
      const FileDescription* fd = process_table[i].fds[j];
      if (fd == nullptr || fd->type != FileType::VfsNode || fd->vfs == nullptr) {
        continue;
      }
      if (fd->vfs->node == node) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace Scheduler
