#include "scheduler.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "address_space.h"
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

    Process *current_process = nullptr;
    Process *ready_head = nullptr;
    Process *ready_tail = nullptr;
    Process *blocked_head = nullptr;
    Process *idle_process = nullptr;

    bool initialized = false;
    bool started = false;

    Process *alloc_process() {
        if (next_pid >= kMaxProcesses) {
            return nullptr;
        }
        Process *p = &process_table[next_pid];
        memset(p, 0, sizeof(Process));
        p->pid = next_pid++;
        p->state = ProcessState::Ready;
        return p;
    }

    void enqueue_ready(Process *p) {
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

    Process *dequeue_ready() {
        if (!ready_head) {
            return nullptr;
        }
        Process *p = ready_head;
        ready_head = ready_head->next;
        if (!ready_head) {
            ready_tail = nullptr;
        }
        p->next = nullptr;
        return p;
    }

    void enqueue_blocked(Process *p) {
        p->state = ProcessState::Blocked;
        p->next = blocked_head;
        blocked_head = p;
    }

    void wake_sleepers() {
        const uint64_t now = PIT::get_ticks();
        Process *prev = nullptr;
        Process *p = blocked_head;

        while (p) {
            Process *next = p->next;
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
    uint32_t switch_to(Process *target) {
        assert(target != nullptr && "switch_to(): null process");

        current_process = target;
        target->state = ProcessState::Running;

        // Update TSS.esp0 so ring-3 interrupts land on this process's kernel
        // stack.
        TSS::set_kernel_stack(reinterpret_cast<uint32_t>(target->kernel_stack)
                              + kKernelStackSize);

        AddressSpace::sync_kernel_mappings(target->page_directory);
        AddressSpace::load(target->page_directory_phys);

        return target->kernel_esp;
    }

}  // namespace

// =============================================================================
// User-space address layout for spawned processes
// =============================================================================

static constexpr vaddr_t kUserStackVA = 0x00BFC000;
static constexpr uint32_t kUserStackPages = 4;
static constexpr vaddr_t kUserStackTop =
    kUserStackVA + kUserStackPages * PAGE_SIZE;

namespace Scheduler {

    void init() {
        assert(!initialized && "Scheduler::init(): called more than once");

        // Create the idle process. This process runs when no other process is
        // ready to run. It uses the kernel's original stack and page tables.
        idle_process = alloc_process();
        assert(idle_process && "Scheduler::init(): failed to allocate idle "
                               "process");
        idle_process->state = ProcessState::Running;
        idle_process->page_directory = &boot_page_directory;
        idle_process->page_directory_phys =
            virt_to_phys(reinterpret_cast<vaddr_t>(&boot_page_directory));
        idle_process->kernel_stack = reinterpret_cast<uint8_t *>(
            reinterpret_cast<uintptr_t>(stack_top) - kKernelStackSize);
        idle_process->kernel_esp = 0;  // will be set on first schedule()

        current_process = idle_process;
        initialized = true;

        // Install the timer interrupt handler. Scheduling is gated by start().
        IDT::set_entry(32, reinterpret_cast<uintptr_t>(timer_entry),
                       IDT::Gate::Interrupt, IDT::Ring::Kernel);
        PIC::unmask(0);
    }

    void start() {
        assert(initialized && "Scheduler::start(): must call init() first");
        started = true;
    }

    bool is_initialized() {
        return initialized;
    }

    Process *create_process(const uint8_t *code, size_t code_len,
                            const uint8_t *data, size_t data_len,
                            vaddr_t entry) {
        assert(initialized
               && "Scheduler::create_process(): scheduler not initialized");

        Process *p = alloc_process();
        if (!p) {
            return nullptr;
        }

        auto [pd_phys, pd_virt] = AddressSpace::create();
        p->page_directory_phys = pd_phys;
        p->page_directory = pd_virt;

        const size_t total_code = code_len + data_len;
        const size_t code_pages = (total_code + PAGE_SIZE - 1) / PAGE_SIZE;
        for (size_t i = 0; i < code_pages; ++i) {
            paddr_t phys = kPmm.alloc();
            assert(phys != 0);
            AddressSpace::map(pd_virt, entry + i * PAGE_SIZE, phys,
                              /*writeable=*/true, /*user=*/true);

            // Copy code/data into the page via the kernel's identity mapping.
            auto *dest = reinterpret_cast<uint8_t *>(phys_to_virt(phys));
            const size_t page_offset = i * PAGE_SIZE;
            size_t to_copy = PAGE_SIZE;
            if (page_offset + to_copy > total_code) {
                to_copy = total_code - page_offset;
            }

            size_t copied = 0;
            if (page_offset < code_len) {
                size_t from_code = code_len - page_offset;
                if (from_code > to_copy) {
                    from_code = to_copy;
                }
                memcpy(dest, code + page_offset, from_code);
                copied = from_code;
            }
            if (copied < to_copy && data && data_len > 0) {
                size_t data_offset = (page_offset + copied > code_len)
                                         ? page_offset + copied - code_len
                                         : 0;
                size_t from_data = to_copy - copied;
                if (from_data > data_len - data_offset) {
                    from_data = data_len - data_offset;
                }
                memcpy(dest + copied, data + data_offset, from_data);
                copied += from_data;
            }
            if (copied < PAGE_SIZE) {
                memset(dest + copied, 0, PAGE_SIZE - copied);
            }
        }

        for (uint32_t i = 0; i < kUserStackPages; ++i) {
            paddr_t phys = kPmm.alloc();
            assert(phys != 0);
            AddressSpace::map(pd_virt, kUserStackVA + i * PAGE_SIZE, phys,
                              /*writeable=*/true, /*user=*/true);
        }

        // Allocate kernel stack for when this process makes system calls.
        p->kernel_stack =
            reinterpret_cast<uint8_t *>(kmalloc(kKernelStackSize));
        assert(p->kernel_stack);
        memset(p->kernel_stack, 0, kKernelStackSize);

        // Set up the initial register state. This TrapFrame is what we'll
        // restore with iret to switch into user mode for the first time.
        auto *kstack_top =
            reinterpret_cast<uint32_t *>(p->kernel_stack + kKernelStackSize);
        auto *frame = reinterpret_cast<TrapFrame *>(
            reinterpret_cast<uintptr_t>(kstack_top) - sizeof(TrapFrame));

        frame->eip = entry;
        frame->cs = GDT::USER_CODE_SELECTOR;
        frame->eflags = 0x202;  // IF set, reserved bit 1 set
        frame->user_esp = kUserStackTop;
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

        p->kernel_esp = reinterpret_cast<uint32_t>(frame);

        enqueue_ready(p);

        printf("Scheduler: created process %u (entry=0x%08x)\n", p->pid,
               static_cast<unsigned>(entry));
        return p;
    }

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

        Process *next = dequeue_ready();
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
        if (current_process->state == ProcessState::Running
            && current_process != idle_process) {
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
        assert(current_process != idle_process
               && "exit_current(): cannot exit idle process");

        printf("Process %u exited with code %u\n", current_process->pid,
               exit_code);

        current_process->state = ProcessState::Zombie;

        // Free address space. Must switch to boot page directory first since
        // we cannot free the currently loaded page directory.
        if (current_process != idle_process) {
            AddressSpace::load(
                virt_to_phys(reinterpret_cast<vaddr_t>(&boot_page_directory)));
            AddressSpace::destroy(current_process->page_directory,
                                  current_process->page_directory_phys);
            current_process->page_directory = nullptr;
            current_process->page_directory_phys = 0;
        }
    }

    void sleep_current(uint32_t ms) {
        assert(current_process != idle_process
               && "sleep_current(): cannot sleep idle process");

        current_process->wake_tick =
            PIT::get_ticks() + (static_cast<uint64_t>(ms) + 9) / 10;
        enqueue_blocked(current_process);
        // The schedule() call will switch away from us.
    }

    Process *current() {
        return current_process;
    }

}  // namespace Scheduler
