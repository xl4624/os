#pragma once

#include <stddef.h>
#include <stdint.h>

#include "process.h"

/*
 * Round-robin preemptive scheduler.
 *
 * The timer IRQ (100 Hz) triggers schedule() on every tick. Processes in
 * the ready queue are run in FIFO order. An idle process (PID 0) runs when
 * no user processes are ready.
 *
 * Context switching works by returning a (possibly different) kernel ESP
 * from schedule(). The assembly stubs (timer_entry.S, syscall_entry.S) use
 * the returned ESP to restore the next process's TrapFrame and iret into it.
 */

namespace Scheduler {

// Create the idle process (PID 0), install timer_entry as the sole
// IRQ 0 handler, and unmask the timer IRQ for PIT tick counting.
// Must be called after heap, PMM, and PIT are initialised.
// Context switching is disabled until start() is called.
void init();

// Enable preemptive context switching. Must be called after init().
void start();

// Check if scheduler has been initialized.
bool is_initialized();

// Create a new user-mode process from an ELF32 executable image.
// Parses the ELF headers, maps PT_LOAD segments into a new address
// space, and sets the entry point from e_entry.
// `name` is placed in argv[0] (argc=1, argv=["name", nullptr]).
// Returns the new process, or nullptr on failure.
Process* create_process(const uint8_t* elf_data, size_t elf_len, const char* name);

// Allocate kUserStackPages physical pages, map them user-writable in pd,
// and write argc=1 / argv=["name", nullptr] onto the stack top.
// Returns the final user_esp, or 0 on allocation failure.
// On failure, any pages already mapped are unmapped and freed.
uint32_t alloc_user_stack(PageTable* pd, const char* name);

// Fill frame with the initial user-mode register state ready for iret.
// GP regs are zeroed; eip/user_esp are set from parameters; eflags=0x202;
// cs/ds/es/fs/gs/ss are set to the GDT user-mode selectors.
void init_trap_frame(TrapFrame* frame, vaddr_t entry, uint32_t user_esp);

// Called from timer_entry.S and syscall_entry.S.
// `esp` is the current kernel stack pointer (pointing to a TrapFrame).
// Returns the kernel ESP to restore (may be a different process's stack).
uint32_t schedule(uint32_t esp);

// Mark the current process as Zombie and free its address space.
// The actual context switch happens when schedule() is called by the
// assembly return path (syscall_entry.S / timer_entry.S).
void exit_current(uint32_t exit_code);

// Block the current process until `ms` milliseconds have elapsed,
// then switch to the next ready process.
void sleep_current(uint32_t ms);

// Block the current process with wake_tick=0, so it wakes on the next
// scheduler tick. Used by the syscall restart mechanism to retry a
// blocked syscall (e.g. pipe read on empty buffer).
void block_current();

// Create a child process as an exact copy of the current process.
// The child gets a deep copy of the parent's address space.
// Returns the child PID to the parent, or (uint32_t)-1 on failure.
// The child's TrapFrame has eax=0 so fork() returns 0 in the child.
uint32_t fork_current(const TrapFrame* parent_regs);

// Block until a child process exits and collect its exit code.
// If pid > 0, wait for that specific child.
// If pid == -1, wait for any child.
// exit_code_ptr, if non-null, receives the child's exit code.
// Returns child PID on success, 0 if blocked (will be restarted by
// scheduler), or -1 if no matching child found.
int32_t waitpid_current(int32_t pid, int32_t* exit_code_ptr);

// Get the currently running process (nullptr before init).
Process* current();

}  // namespace Scheduler
