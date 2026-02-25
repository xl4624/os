#pragma once

#include <stdint.h>

#include "paging.h"

struct Process;

// Maximum shared memory regions system-wide.
static constexpr uint32_t kMaxShmRegions = 32;

// Maximum physical pages per shared memory region (64 KiB).
static constexpr uint32_t kMaxShmPages = 16;

// Maximum shared memory mappings per process.
static constexpr uint32_t kMaxShmMappings = 8;

struct ShmRegion {
  bool in_use;
  uint32_t id;
  paddr_t pages[kMaxShmPages];
  uint32_t num_pages;
  uint32_t ref_count;  // number of processes currently attached
};

// Per-process record of a shared memory mapping.
struct ShmMapping {
  uint32_t shm_id;
  vaddr_t vaddr;
  uint32_t num_pages;
};

namespace Shm {

// Allocate ceil(size / PAGE_SIZE) physical pages and create a shared
// memory region. Returns the shmid (>= 0) on success, -1 on failure.
int32_t create(uint32_t size);

// Map the shared memory region into the calling process's address space
// at the given virtual address. Returns 0 on success, -1 on failure.
int32_t attach(uint32_t shmid, vaddr_t vaddr);

// Unmap the shared memory region starting at vaddr (size bytes) from
// the calling process. Does not free the physical pages unless this
// was the last attachment. Returns 0 on success, -1 on failure.
int32_t detach(vaddr_t vaddr, uint32_t size);

// Detach all shared memory from proc. Called during process exit
// before AddressSpace::destroy() so that shared pages are not
// double-freed.
void detach_all(Process* proc);

// Look up a region by id. Returns nullptr if not found.
ShmRegion* find_region(uint32_t shmid);

}  // namespace Shm
