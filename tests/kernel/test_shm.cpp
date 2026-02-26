#include "address_space.h"
#include "ktest.h"
#include "paging.h"
#include "pmm.h"
#include "process.h"
#include "scheduler.h"
#include "shm.h"

// ===========================================================================
// Shm::create
// ===========================================================================

TEST(shm, create_returns_valid_id) {
  int32_t id = Shm::create(PAGE_SIZE);
  ASSERT(id >= 0);

  // Clean up by finding and freeing.
  ShmRegion* region = Shm::find_region(static_cast<uint32_t>(id));
  ASSERT_NOT_NULL(region);
  for (uint32_t i = 0; i < region->num_pages; ++i) {
    kPmm.free(region->pages[i]);
  }
  region->in_use = false;
}

TEST(shm, create_zero_size_fails) {
  int32_t id = Shm::create(0);
  ASSERT_EQ(id, -1);
}

TEST(shm, create_rounds_up_to_page) {
  int32_t id = Shm::create(1);
  ASSERT(id >= 0);

  ShmRegion* region = Shm::find_region(static_cast<uint32_t>(id));
  ASSERT_NOT_NULL(region);
  ASSERT_EQ(region->num_pages, 1u);

  for (uint32_t i = 0; i < region->num_pages; ++i) {
    kPmm.free(region->pages[i]);
  }
  region->in_use = false;
}

// ===========================================================================
// Shm::attach / Shm::detach
// ===========================================================================

TEST(shm, attach_maps_pages) {
  auto [pd_phys, pd] = AddressSpace::create();
  ASSERT_NOT_NULL(pd);

  Process* proc = Scheduler::current();
  PageTable* orig_pd = proc->page_directory;
  paddr_t orig_pd_phys = proc->page_directory_phys;
  proc->page_directory = pd;
  proc->page_directory_phys = pd_phys;

  int32_t id = Shm::create(PAGE_SIZE);
  ASSERT(id >= 0);

  vaddr_t va{0x00A00000};
  int32_t ret = Shm::attach(static_cast<uint32_t>(id), va);
  ASSERT_EQ(ret, 0);
  ASSERT_TRUE(AddressSpace::is_user_mapped(pd, va, /*writeable=*/true));

  // Detach.
  ASSERT_NE(Shm::detach(va, PAGE_SIZE), -1);
  ASSERT_FALSE(AddressSpace::is_user_mapped(pd, va, /*writeable=*/false));

  proc->page_directory = orig_pd;
  proc->page_directory_phys = orig_pd_phys;
  AddressSpace::destroy(pd, pd_phys);
}

TEST(shm, detach_preserves_physical_pages_with_refs) {
  auto [pd_phys, pd] = AddressSpace::create();
  ASSERT_NOT_NULL(pd);

  Process* proc = Scheduler::current();
  PageTable* orig_pd = proc->page_directory;
  paddr_t orig_pd_phys = proc->page_directory_phys;
  proc->page_directory = pd;
  proc->page_directory_phys = pd_phys;

  int32_t id = Shm::create(PAGE_SIZE);
  ASSERT(id >= 0);

  ShmRegion* region = Shm::find_region(static_cast<uint32_t>(id));
  ASSERT_NOT_NULL(region);

  // Attach twice (simulating two processes by incrementing ref_count).
  vaddr_t va{0x00A00000};
  ASSERT_NE(Shm::attach(static_cast<uint32_t>(id), va), -1);
  ASSERT_EQ(region->ref_count, 1u);

  // Manually bump ref_count to simulate a second attachment.
  ++region->ref_count;

  // Detach once: region should still be alive.
  ASSERT_NE(Shm::detach(va, PAGE_SIZE), -1);
  ASSERT_EQ(region->ref_count, 1u);
  ASSERT_TRUE(region->in_use);

  // Final cleanup: decrement and free.
  --region->ref_count;
  for (uint32_t i = 0; i < region->num_pages; ++i) {
    kPmm.free(region->pages[i]);
  }
  region->in_use = false;

  proc->page_directory = orig_pd;
  proc->page_directory_phys = orig_pd_phys;
  AddressSpace::destroy(pd, pd_phys);
}

TEST(shm, last_detach_frees_region) {
  auto [pd_phys, pd] = AddressSpace::create();
  ASSERT_NOT_NULL(pd);

  Process* proc = Scheduler::current();
  PageTable* orig_pd = proc->page_directory;
  paddr_t orig_pd_phys = proc->page_directory_phys;
  proc->page_directory = pd;
  proc->page_directory_phys = pd_phys;

  int32_t id = Shm::create(PAGE_SIZE);
  ASSERT(id >= 0);

  ShmRegion* region = Shm::find_region(static_cast<uint32_t>(id));
  ASSERT_NOT_NULL(region);

  vaddr_t va{0x00A00000};
  ASSERT_NE(Shm::attach(static_cast<uint32_t>(id), va), -1);
  ASSERT_EQ(region->ref_count, 1u);

  // Detach the only reference: region should be freed.
  ASSERT_NE(Shm::detach(va, PAGE_SIZE), -1);
  ASSERT_FALSE(region->in_use);

  proc->page_directory = orig_pd;
  proc->page_directory_phys = orig_pd_phys;
  AddressSpace::destroy(pd, pd_phys);
}

// ===========================================================================
// AddressSpace::unmap_nofree
// ===========================================================================

TEST(shm, unmap_nofree_preserves_physical_page) {
  auto [pd_phys, pd] = AddressSpace::create();
  ASSERT_NOT_NULL(pd);

  paddr_t phys = kPmm.alloc();
  ASSERT(phys != 0);

  vaddr_t va{0x00B00000};
  AddressSpace::map(pd, va, phys, /*writeable=*/true, /*user=*/true);
  ASSERT_TRUE(AddressSpace::is_user_mapped(pd, va, /*writeable=*/true));

  // unmap_nofree removes the mapping but does not free the physical page.
  AddressSpace::unmap_nofree(pd, va);
  ASSERT_FALSE(AddressSpace::is_user_mapped(pd, va, /*writeable=*/false));

  // The physical page is still allocated (not freed), so we must free it.
  kPmm.free(phys);

  AddressSpace::destroy(pd, pd_phys);
}
