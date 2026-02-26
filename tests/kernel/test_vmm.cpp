#include <stdint.h>

#include "ktest.h"
#include "paging.h"
#include "pmm.h"
#include "vmm.h"

// Virtual addresses in PDE 770 (0xC0800000-0xC0BFFFFF), past the boot-mapped
// 8 MiB window, so every test that maps a page here causes VMM to allocate a
// fresh page table on first use.
static constexpr vaddr_t BASE = 0xC0800000;

// ===========================================================================
// VMM::map / VMM::get_phys
// ===========================================================================

TEST(vmm, get_phys_unmapped_returns_zero) {
  // PDE 771 has never been touched; no page table exists for it.
  static constexpr vaddr_t UNMAPPED = 0xC0C00000;
  ASSERT_EQ(VMM::get_phys(UNMAPPED), static_cast<paddr_t>(0));
}

TEST(vmm, map_then_get_phys) {
  const paddr_t phys = kPmm.alloc();
  ASSERT_NE(phys, static_cast<paddr_t>(0));

  VMM::map(BASE, phys);
  ASSERT_EQ(VMM::get_phys(BASE), phys);

  VMM::unmap(BASE);
  kPmm.free(phys);
}

TEST(vmm, write_through_mapped_page) {
  const paddr_t phys = kPmm.alloc();
  ASSERT_NE(phys, static_cast<paddr_t>(0));

  static constexpr vaddr_t VA = BASE + 2 * PAGE_SIZE;
  VMM::map(VA, phys);

  // Write through the virtual address and verify the value persists.
  volatile uint32_t* ptr = VA.ptr<volatile uint32_t>();
  *ptr = 0xDEADBEEF;
  ASSERT_EQ(*ptr, static_cast<uint32_t>(0xDEADBEEF));

  VMM::unmap(VA);
  kPmm.free(phys);
}

TEST(vmm, remap_to_different_frame) {
  const paddr_t phys1 = kPmm.alloc();
  const paddr_t phys2 = kPmm.alloc();
  ASSERT_NE(phys1, static_cast<paddr_t>(0));
  ASSERT_NE(phys2, static_cast<paddr_t>(0));

  static constexpr vaddr_t VA = BASE + 3 * PAGE_SIZE;
  VMM::map(VA, phys1);
  ASSERT_EQ(VMM::get_phys(VA), phys1);

  VMM::map(VA, phys2);  // remap the same virtual page to a different frame
  ASSERT_EQ(VMM::get_phys(VA), phys2);

  VMM::unmap(VA);
  kPmm.free(phys1);
  kPmm.free(phys2);
}

TEST(vmm, map_contiguous_range) {
  static constexpr int N = 4;
  paddr_t phys[N];

  for (int i = 0; i < N; ++i) {
    phys[i] = kPmm.alloc();
    ASSERT_NE(phys[i], static_cast<paddr_t>(0));
    const vaddr_t va = BASE + static_cast<vaddr_t>(4 + i) * PAGE_SIZE;
    VMM::map(va, phys[i]);
  }

  for (int i = 0; i < N; ++i) {
    const vaddr_t va = BASE + static_cast<vaddr_t>(4 + i) * PAGE_SIZE;
    ASSERT_EQ(VMM::get_phys(va), phys[i]);
    VMM::unmap(va);
    kPmm.free(phys[i]);
  }
}

TEST(vmm, get_phys_preserves_offset) {
  const paddr_t phys = kPmm.alloc();
  ASSERT_NE(phys, static_cast<paddr_t>(0));

  static constexpr vaddr_t VA = BASE + 8 * PAGE_SIZE;
  VMM::map(VA, phys);

  // get_phys with a non-zero offset within the page.
  static constexpr vaddr_t offset = 0x123;
  ASSERT_EQ(VMM::get_phys(VA + offset), phys + offset);

  VMM::unmap(VA);
  kPmm.free(phys);
}

// ===========================================================================
// VMM::unmap
// ===========================================================================

TEST(vmm, unmap_clears_mapping) {
  const paddr_t phys = kPmm.alloc();
  ASSERT_NE(phys, static_cast<paddr_t>(0));

  static constexpr vaddr_t VA = BASE + PAGE_SIZE;
  VMM::map(VA, phys);
  VMM::unmap(VA);
  ASSERT_EQ(VMM::get_phys(VA), static_cast<paddr_t>(0));

  kPmm.free(phys);
}

TEST(vmm, unmap_idempotent) {
  const paddr_t phys = kPmm.alloc();
  ASSERT_NE(phys, static_cast<paddr_t>(0));

  static constexpr vaddr_t VA = BASE + 9 * PAGE_SIZE;
  VMM::map(VA, phys);
  VMM::unmap(VA);
  VMM::unmap(VA);  // second unmap must not panic
  kPmm.free(phys);
}

// Adjacent virtual pages must map and report their respective frames
// independently -- no aliasing between neighboring PTEs.
TEST(vmm, adjacent_pages_are_independent) {
  const paddr_t phys1 = kPmm.alloc();
  const paddr_t phys2 = kPmm.alloc();
  ASSERT_NE(phys1, static_cast<paddr_t>(0));
  ASSERT_NE(phys2, static_cast<paddr_t>(0));

  static constexpr vaddr_t VA1 = BASE + 20 * PAGE_SIZE;
  static constexpr vaddr_t VA2 = BASE + 21 * PAGE_SIZE;
  VMM::map(VA1, phys1);
  VMM::map(VA2, phys2);
  ASSERT_EQ(VMM::get_phys(VA1), phys1);
  ASSERT_EQ(VMM::get_phys(VA2), phys2);

  VMM::unmap(VA1);
  VMM::unmap(VA2);
  kPmm.free(phys1);
  kPmm.free(phys2);
}

// Unmapping one page must leave its neighbor intact.
TEST(vmm, unmap_does_not_affect_neighbor) {
  const paddr_t phys1 = kPmm.alloc();
  const paddr_t phys2 = kPmm.alloc();
  ASSERT_NE(phys1, static_cast<paddr_t>(0));
  ASSERT_NE(phys2, static_cast<paddr_t>(0));

  static constexpr vaddr_t VA1 = BASE + 22 * PAGE_SIZE;
  static constexpr vaddr_t VA2 = BASE + 23 * PAGE_SIZE;
  VMM::map(VA1, phys1);
  VMM::map(VA2, phys2);

  VMM::unmap(VA1);
  ASSERT_EQ(VMM::get_phys(VA1), static_cast<paddr_t>(0));
  ASSERT_EQ(VMM::get_phys(VA2), phys2);  // neighbor must be unaffected

  VMM::unmap(VA2);
  kPmm.free(phys1);
  kPmm.free(phys2);
}
