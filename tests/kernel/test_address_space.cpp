#include "address_space.h"
#include "ktest.h"
#include "paging.h"
#include "pmm.h"

// These tests focus on AddressSpace::is_user_mapped and AddressSpace::unmap.
// The create/destroy and basic map tests live in test_scheduler.cpp.

// ===========================================================================
// AddressSpace::is_user_mapped
// ===========================================================================

TEST(address_space, is_user_mapped_unmapped_returns_false) {
  auto [pd_phys, pd] = AddressSpace::create();
  // 0x00400000 is never touched in a fresh address space.
  ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
  ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/true));
  AddressSpace::destroy(pd, pd_phys);
}

TEST(address_space, is_user_mapped_mapped_returns_true) {
  auto [pd_phys, pd] = AddressSpace::create();
  paddr_t page = kPmm.alloc();
  ASSERT_NE(page, 0u);
  AddressSpace::map(pd, 0x00400000, page, /*writeable=*/true, /*user=*/true);
  ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
  ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/true));
  AddressSpace::destroy(pd, pd_phys);
}

TEST(address_space, is_user_mapped_read_only) {
  auto [pd_phys, pd] = AddressSpace::create();
  paddr_t page = kPmm.alloc();
  ASSERT_NE(page, 0u);
  AddressSpace::map(pd, 0x00400000, page, /*writeable=*/false, /*user=*/true);
  // Read-only page: non-writeable check passes, writeable check fails.
  ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
  ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/true));
  AddressSpace::destroy(pd, pd_phys);
}

// ===========================================================================
// AddressSpace::unmap
// ===========================================================================

TEST(address_space, unmap_removes_mapping) {
  auto [pd_phys, pd] = AddressSpace::create();
  paddr_t page = kPmm.alloc();
  ASSERT_NE(page, 0u);
  AddressSpace::map(pd, 0x00400000, page, /*writeable=*/true, /*user=*/true);
  ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
  AddressSpace::unmap(pd, 0x00400000);
  // Page was freed by unmap; is_user_mapped must now return false.
  ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
  AddressSpace::destroy(pd, pd_phys);
}

TEST(address_space, is_user_mapped_offset_within_page) {
  auto [pd_phys, pd] = AddressSpace::create();
  paddr_t page = kPmm.alloc();
  ASSERT_NE(page, 0u);
  AddressSpace::map(pd, 0x00400000, page, /*writeable=*/true, /*user=*/true);
  // is_user_mapped rounds va down to its page; the last byte of the same
  // page should pass, and the first byte of the next page must fail.
  ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400FFF, /*writeable=*/false));
  ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00401000, /*writeable=*/false));
  AddressSpace::destroy(pd, pd_phys);
}

TEST(address_space, unmap_noop_on_unmapped) {
  auto [pd_phys, pd] = AddressSpace::create();
  // Unmapping an address that was never mapped must not crash.
  AddressSpace::unmap(pd, 0x00600000);
  ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00600000, /*writeable=*/false));
  AddressSpace::destroy(pd, pd_phys);
}

// ===========================================================================
// AddressSpace
// ===========================================================================

TEST(address_space, sync_kernel_mappings) {
  auto [pd_phys, pd] = AddressSpace::create();
  // After syncing, kernel PDEs (768-1023) must match boot_page_directory.
  AddressSpace::sync_kernel_mappings(pd);
  for (uint32_t i = AddressSpace::kKernelPdeStart; i < PAGES_PER_TABLE; ++i) {
    ASSERT_EQ(pd->entry[i].present, boot_page_directory.entry[i].present);
    if (pd->entry[i].present) {
      ASSERT_EQ(pd->entry[i].frame, boot_page_directory.entry[i].frame);
    }
  }
  AddressSpace::destroy(pd, pd_phys);
}

// Mapping N pages at consecutive virtual addresses must make each address
// individually user-accessible. destroy() frees all pages on teardown.
TEST(address_space, map_multiple_pages) {
  auto [pd_phys, pd] = AddressSpace::create();
  static constexpr int N = 8;
  for (int i = 0; i < N; ++i) {
    paddr_t page = kPmm.alloc();
    ASSERT_NE(page, 0u);
    AddressSpace::map(pd, static_cast<vaddr_t>(0x00400000 + i * PAGE_SIZE), page,
                      /*writeable=*/true, /*user=*/true);
  }
  for (int i = 0; i < N; ++i) {
    ASSERT_TRUE(AddressSpace::is_user_mapped(pd, static_cast<vaddr_t>(0x00400000 + i * PAGE_SIZE),
                                             /*writeable=*/false));
  }
  AddressSpace::destroy(pd, pd_phys);
}

// A user address just below the kernel boundary must not be considered
// kernel-accessible (regression check for the user flag in PTEs).
TEST(address_space, kernel_boundary_not_user_mapped_before_mapping) {
  auto [pd_phys, pd] = AddressSpace::create();
  // Address 0xBFFFE000 is the last page below KERNEL_VMA in many layouts.
  // Before any explicit map, it must not be user-mapped.
  static constexpr vaddr_t NEAR_KERNEL = KERNEL_VMA - PAGE_SIZE;
  ASSERT_FALSE(AddressSpace::is_user_mapped(pd, NEAR_KERNEL,
                                            /*writeable=*/false));
  AddressSpace::destroy(pd, pd_phys);
}
