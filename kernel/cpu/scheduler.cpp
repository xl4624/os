#include "scheduler.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "address_space.h"
#include "elf.h"
#include "gdt.h"
#include "heap.h"
#include "idt.h"
#include "paging.h"
#include "pic.h"
#include "pit.h"
#include "pmm.h"
#include "tss.h"

__BEGIN_DECLS

// Assembly entry point defined in timer_entry.S.
void timer_entry();

// Kernel stack symbol from arch/boot.S.
extern char stack_top[];

uint32_t timer_dispatch(uint32_t esp) {
  PIT::tick();
  PIC::send_eoi(0);
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

Process process_table[kMaxProcesses];
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
  return p;
}

void enqueue_ready(Process* p) {
  assert(p != nullptr && "enqueue_ready(): null process");
  p->next = nullptr;
  p->state = ProcessState::Ready;
  if (!ready_tail) {
    ready_head = ready_tail = p;
  } else {
    ready_tail->next = p;
    ready_tail = p;
  }
}

Process* dequeue_ready() {
  if (!ready_head) {
    return nullptr;
  }
  Process* p = ready_head;
  ready_head = ready_head->next;
  if (!ready_head) {
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

  while (p) {
    Process* next = p->next;
    if (now >= p->wake_tick) {
      if (prev) {
        prev->next = next;
      } else {
        blocked_head = next;
      }
      enqueue_ready(p);
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

  // Update TSS.esp0 so ring-3 interrupts land on this process's kernel
  // stack.
  TSS::set_kernel_stack(reinterpret_cast<uint32_t>(target->kernel_stack) + kKernelStackSize);

  AddressSpace::sync_kernel_mappings(target->page_directory);
  AddressSpace::load(target->page_directory_phys);

  return target->kernel_esp;
}

}  // namespace

namespace Scheduler {

void init() {
  assert(!initialized && "Scheduler::init(): called more than once");

  // Create the idle process. This process runs when no other process is
  // ready to run. It uses the kernel's original stack and page tables.
  idle_process = alloc_process();
  assert(idle_process &&
         "Scheduler::init(): failed to allocate idle "
         "process");
  idle_process->state = ProcessState::Running;
  idle_process->page_directory = &boot_page_directory;
  idle_process->page_directory_phys = virt_to_phys(vaddr_t{&boot_page_directory});
  idle_process->kernel_stack =
      reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(stack_top) - kKernelStackSize);
  idle_process->kernel_esp = 0;  // will be set on first schedule()

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
  if (!next) {
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

  current_process->wake_tick = PIT::get_ticks() + (static_cast<uint64_t>(ms) + 9) / 10;
  enqueue_blocked(current_process);
  // The schedule() call will switch away from us.
}

uint32_t alloc_user_stack(PageTable* pd, const char* name) {
  paddr_t stack_pages[kUserStackPages];
  for (uint32_t i = 0; i < kUserStackPages; ++i) {
    paddr_t phys = kPmm.alloc();
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
    uint32_t idx = (uva - kUserStackVA) / PAGE_SIZE;
    uint32_t off = (uva - kUserStackVA) % PAGE_SIZE;
    return phys_to_virt(stack_pages[idx]).ptr<uint8_t>() + off;
  };

  uint32_t user_esp = kUserStackTop;

  // Push the program name string (null-terminated).
  size_t name_len = strlen(name);
  user_esp -= static_cast<uint32_t>(name_len + 1);
  memcpy(kptr(user_esp), name, name_len + 1);
  uint32_t str_uva = user_esp;

  // Align esp down to a 4-byte boundary.
  user_esp &= ~3u;

  // Push argv[1] = NULL (argv terminator).
  user_esp -= 4;
  *reinterpret_cast<uint32_t*>(kptr(user_esp)) = 0;

  // Push argv[0] = pointer to the name string.
  user_esp -= 4;
  *reinterpret_cast<uint32_t*>(kptr(user_esp)) = str_uva;

  // Push argc = 1.
  user_esp -= 4;
  *reinterpret_cast<uint32_t*>(kptr(user_esp)) = 1;

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

Process* create_process(const uint8_t* elf_data, size_t elf_len, const char* name) {
  assert(initialized &&
         "Scheduler::create_process(): scheduler not "
         "initialized");

  if (!name) {
    name = "";
  }

  Process* p = alloc_process();
  if (!p) {
    printf("Scheduler: failed to allocate process\n");
    return nullptr;
  }

  auto [pd_phys, pd_virt] = AddressSpace::create();
  p->page_directory_phys = pd_phys;
  p->page_directory = pd_virt;

  vaddr_t entry = 0;
  vaddr_t brk = 0;
  if (!Elf::load(elf_data, elf_len, pd_virt, entry, brk)) {
    printf("Scheduler: failed to load ELF for process %u\n", p->pid);
    AddressSpace::destroy(pd_virt, pd_phys);
    p->state = ProcessState::Zombie;
    return nullptr;
  }
  p->heap_break = brk;

  // Allocate user stack and write argc/argv onto it.
  uint32_t user_esp = alloc_user_stack(pd_virt, name);
  assert(user_esp != 0 &&
         "Scheduler::create_process(): out of physical memory "
         "for user stack");

  // Allocate kernel stack for system calls and interrupts.
  p->kernel_stack = reinterpret_cast<uint8_t*>(kmalloc(kKernelStackSize));
  assert(p->kernel_stack &&
         "Scheduler::create_process(): failed to allocate kernel "
         "stack");
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
  if (!child) {
    return static_cast<uint32_t>(-1);
  }

  auto [child_pd_phys, child_pd] = AddressSpace::copy(current_process->page_directory);
  child->page_directory_phys = child_pd_phys;
  child->page_directory = child_pd;
  child->heap_break = current_process->heap_break;
  child->parent_pid = current_process->pid;

  child->kernel_stack = reinterpret_cast<uint8_t*>(kmalloc(kKernelStackSize));
  assert(child->kernel_stack && "fork_current(): failed to allocate kernel stack");
  memset(child->kernel_stack, 0, kKernelStackSize);

  // Place a copy of the parent's TrapFrame at the top of the child's
  // kernel stack. The child returns 0 from fork().
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

  while (true) {
    for (uint32_t i = 1; i < next_pid; ++i) {
      Process& child = process_table[i];
      if (child.parent_pid == current_process->pid && child.state == ProcessState::Zombie) {
        if (exit_code_ptr) {
          *exit_code_ptr = child.exit_code;
        }
        uint32_t child_pid = child.pid;
        memset(&child, 0, sizeof(Process));
        return static_cast<int32_t>(child_pid);
      }
    }

    if (pid > 0) {
      return -1;
    }

    enqueue_blocked(current_process);
    return 0;
  }
}

Process* current() { return current_process; }

}  // namespace Scheduler
