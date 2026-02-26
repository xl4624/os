#include "shm.h"

#include <array.h>
#include <string.h>

#include "address_space.h"
#include "pmm.h"
#include "process.h"
#include "scheduler.h"

namespace {

std::array<ShmRegion, kMaxShmRegions> regions;
uint32_t next_id = 0;

}  // namespace

namespace Shm {

int32_t create(uint32_t size) {
  if (size == 0) {
    return -1;
  }

  uint32_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  if (num_pages > kMaxShmPages) {
    return -1;
  }

  // Find a free slot.
  ShmRegion* region = nullptr;
  for (auto& r : regions) {
    if (!r.in_use) {
      region = &r;
      break;
    }
  }
  if (!region) {
    return -1;
  }

  // Allocate physical pages.
  for (uint32_t i = 0; i < num_pages; ++i) {
    paddr_t phys = kPmm.alloc();
    if (phys == 0) {
      for (uint32_t j = 0; j < i; ++j) {
        kPmm.free(region->pages[j]);
      }
      return -1;
    }
    region->pages[i] = phys;

    memset(phys_to_virt(phys).ptr<uint8_t>(), 0, PAGE_SIZE);
  }

  region->in_use = true;
  region->id = next_id++;
  region->num_pages = num_pages;
  region->ref_count = 0;

  return static_cast<int32_t>(region->id);
}

int32_t attach(uint32_t shmid, vaddr_t vaddr) {
  ShmRegion* region = find_region(shmid);
  if (!region) {
    return -1;
  }

  // Validate alignment and user-space range.
  if (vaddr % PAGE_SIZE != 0) {
    return -1;
  }
  vaddr_t end = vaddr + region->num_pages * PAGE_SIZE;
  if (end <= vaddr || end > KERNEL_VMA) {
    return -1;
  }

  Process* proc = Scheduler::current();

  // Check for room in the per-process mapping table.
  if (proc->shm_mapping_count >= kMaxShmMappings) {
    return -1;
  }

  // Map each page into the process.
  for (uint32_t i = 0; i < region->num_pages; ++i) {
    AddressSpace::map(proc->page_directory, vaddr + i * PAGE_SIZE, region->pages[i],
                      /*writeable=*/true, /*user=*/true);
  }

  // Record the mapping.
  ShmMapping& m = proc->shm_mappings[proc->shm_mapping_count++];
  m.shm_id = region->id;
  m.vaddr = vaddr;
  m.num_pages = region->num_pages;

  ++region->ref_count;
  return 0;
}

int32_t detach(vaddr_t vaddr, uint32_t size) {
  Process* proc = Scheduler::current();

  // Find the mapping by vaddr.
  for (uint32_t i = 0; i < proc->shm_mapping_count; ++i) {
    ShmMapping& m = proc->shm_mappings[i];
    if (m.vaddr != vaddr) {
      continue;
    }

    uint32_t expected_size = m.num_pages * PAGE_SIZE;
    if (size != expected_size) {
      return -1;
    }

    // Unmap pages without freeing the physical frames.
    for (uint32_t p = 0; p < m.num_pages; ++p) {
      AddressSpace::unmap_nofree(proc->page_directory, vaddr + p * PAGE_SIZE);
    }

    ShmRegion* region = find_region(m.shm_id);

    // Remove mapping by swapping with last entry.
    proc->shm_mappings[i] = proc->shm_mappings[--proc->shm_mapping_count];

    // Decrement ref count. If last reference, free physical pages.
    if (region && region->ref_count > 0) {
      --region->ref_count;
      if (region->ref_count == 0) {
        for (uint32_t p = 0; p < region->num_pages; ++p) {
          kPmm.free(region->pages[p]);
        }
        region->in_use = false;
      }
    }

    return 0;
  }

  return -1;
}

void detach_all(Process* proc) {
  while (proc->shm_mapping_count > 0) {
    ShmMapping& m = proc->shm_mappings[0];

    // Unmap without freeing physical frames.
    for (uint32_t p = 0; p < m.num_pages; ++p) {
      AddressSpace::unmap_nofree(proc->page_directory, m.vaddr + p * PAGE_SIZE);
    }

    ShmRegion* region = find_region(m.shm_id);

    // Remove mapping.
    proc->shm_mappings[0] = proc->shm_mappings[--proc->shm_mapping_count];

    if (region && region->ref_count > 0) {
      --region->ref_count;
      if (region->ref_count == 0) {
        for (uint32_t p = 0; p < region->num_pages; ++p) {
          kPmm.free(region->pages[p]);
        }
        region->in_use = false;
      }
    }
  }
}

ShmRegion* find_region(uint32_t shmid) {
  for (auto& r : regions) {
    if (r.in_use && r.id == shmid) {
      return &r;
    }
  }
  return nullptr;
}

}  // namespace Shm
