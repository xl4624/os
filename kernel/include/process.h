#pragma once

#include <array.h>
#include <stddef.h>
#include <stdint.h>

#include "file.h"
#include "paging.h"
#include "shm.h"

/*
 * Trap frame: full register state saved on the kernel stack when a process
 * is interrupted (by a hardware IRQ or int 0x80 syscall). The layout matches
 * the push order in timer_entry.S and syscall_entry.S:
 *
 *   [low addr]  pushal regs → segment regs → CPU-pushed iret frame  [high addr]
 *
 * Fields are ordered from LOW address (ESP after all pushes) to HIGH address,
 * which is the reverse of the push order.
 */
struct TrapFrame {
  // Pushed last by pushal (EDI at lowest address, EAX at highest).
  uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
  // Pushed before pushal: ds pushed last (lowest), gs pushed first (highest).
  uint32_t ds, es, fs, gs;
  // Pushed by CPU on privilege-level change (EIP lowest, SS highest).
  uint32_t eip, cs, eflags, user_esp, user_ss;
} __attribute__((packed));

static_assert(sizeof(TrapFrame) == 17 * 4, "TrapFrame must be 17 dwords (68 bytes)");
static_assert(offsetof(TrapFrame, edi) == 0, "edi at offset 0");
static_assert(offsetof(TrapFrame, eax) == 28, "eax at offset 28");
static_assert(offsetof(TrapFrame, ds) == 32, "ds at offset 32");
static_assert(offsetof(TrapFrame, eip) == 48, "eip at offset 48");
static_assert(offsetof(TrapFrame, user_ss) == 64, "user_ss at offset 64");

// Process states for the scheduler.
enum class ProcessState : uint8_t {
  Ready,
  Running,
  Blocked,
  Zombie,
};

// Per-process kernel stack size (2 pages = 8 KiB).
static constexpr uint32_t kKernelStackSize = 8192;

// Maximum number of processes the system can track.
static constexpr uint32_t kMaxProcesses = 64;

// User-space stack layout for newly created processes.
static constexpr vaddr_t kUserStackVA = 0x00BFC000;
static constexpr uint32_t kUserStackPages = 4;
static constexpr vaddr_t kUserStackTop = kUserStackVA + kUserStackPages * PAGE_SIZE;

/*
 * Process Control Block (PCB).
 *
 * Each process has its own kernel stack, address space (page directory),
 * and saved register state. The scheduler maintains intrusive linked lists
 * of processes via the `next` pointer.
 */
struct Process {
  uint32_t pid;
  uint32_t parent_pid;  // pid of the parent process (0 for root processes)
  ProcessState state;
  uint32_t kernel_esp;                          // saved kernel stack pointer (into kernel_stack)
  paddr_t page_directory_phys;                  // CR3 value for this process
  PageTable* page_directory;                    // virtual pointer to page directory
  uint8_t* kernel_stack;                        // base of allocated kernel stack (for cleanup)
  vaddr_t heap_break;                           // current program break for sbrk
  uint64_t wake_tick;                           // tick at which a sleeping process should wake
  int32_t exit_code;                            // exit code stored when process becomes zombie
  std::array<FileDescription*, kMaxFds> fds{};  // per-process file descriptor table
  std::array<ShmMapping, kMaxShmMappings> shm_mappings{};  // shared memory attachments
  uint32_t shm_mapping_count;                              // number of active shm mappings
  Process* next;  // intrusive list pointer (ready/blocked queues)
};
