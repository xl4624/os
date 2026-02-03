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
    // Returns the new process, or nullptr on failure.
    Process *create_process(const uint8_t *elf_data, size_t elf_len);

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

    // Get the currently running process (nullptr before init).
    Process *current();

}  // namespace Scheduler
