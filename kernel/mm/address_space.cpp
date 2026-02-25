#include "address_space.h"

#include <assert.h>
#include <string.h>

#include "panic.h"
#include "pmm.h"

namespace {

// Page table pages must reside in the first 8 MiB (accessible via
// phys_to_virt). The PMM allocates lowest frames first so this holds.
constexpr paddr_t kMappedPhysEnd = 8u * 1024u * 1024u;

// Top 10 bits of a virtual address → page directory index.
constexpr uint32_t pd_index(vaddr_t va) { return va >> (PAGE_TABLE_BITS + PAGE_OFFSET_BITS); }

// Middle 10 bits of a virtual address -> page table index.
constexpr uint32_t pt_index(vaddr_t va) { return (va >> PAGE_OFFSET_BITS) & (PAGES_PER_TABLE - 1); }

// Extract the physical address from a PDE/PTE frame field.
constexpr paddr_t frame_to_phys(uint32_t frame) { return paddr_t{frame} << PAGE_OFFSET_BITS; }

}  // namespace

namespace AddressSpace {

PageDir create() {
  const paddr_t pd_phys = kPmm.alloc();
  assert(pd_phys && "AddressSpace::create(): out of physical memory\n");
  if (pd_phys >= kMappedPhysEnd) {
    panic(
        "AddressSpace::create(): page dir phys 0x%08x outside mapped "
        "region\n",
        static_cast<unsigned>(pd_phys));
  }

  auto* pd = phys_to_virt(pd_phys).ptr<PageTable>();

  memset(pd->entry, 0, kKernelPdeStart * sizeof(PageEntry));

  memcpy(&pd->entry[kKernelPdeStart], &boot_page_directory.entry[kKernelPdeStart],
         (PAGES_PER_TABLE - kKernelPdeStart) * sizeof(PageEntry));

  return {pd_phys, pd};
}

void map(PageTable* pd, vaddr_t virt, paddr_t phys, bool writeable, bool user) {
  const uint32_t pdi = pd_index(virt);
  PageEntry& pde = pd->entry[pdi];

  if (!pde.present) {
    const paddr_t pt_phys = kPmm.alloc();
    assert(pt_phys &&
           "AddressSpace::map(): out of physical memory for page "
           "table\n");
    if (pt_phys >= kMappedPhysEnd) {
      panic(
          "AddressSpace::map(): page table phys 0x%08x outside "
          "mapped region\n",
          static_cast<unsigned>(pt_phys));
    }

    auto* pt = phys_to_virt(pt_phys).ptr<PageTable>();
    memset(pt, 0, sizeof(PageTable));
    pde = PageEntry(pt_phys, /*writeable=*/true, /*user=*/user);
  }

  const paddr_t pt_phys = frame_to_phys(pde.frame);
  auto* pt = phys_to_virt(pt_phys).ptr<PageTable>();
  pt->entry[pt_index(virt)] = PageEntry(phys, writeable, user);
}

void unmap(PageTable* pd, vaddr_t virt) {
  const uint32_t pdi = pd_index(virt);
  PageEntry& pde = pd->entry[pdi];
  if (!pde.present) {
    return;
  }

  const paddr_t pt_phys = frame_to_phys(pde.frame);
  auto* pt = phys_to_virt(pt_phys).ptr<PageTable>();
  PageEntry& pte = pt->entry[pt_index(virt)];
  if (!pte.present) {
    return;
  }

  kPmm.free(frame_to_phys(pte.frame));
  memset(&pte, 0, sizeof(pte));
  asm volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void unmap_nofree(PageTable* pd, vaddr_t virt) {
  const uint32_t pdi = pd_index(virt);
  PageEntry& pde = pd->entry[pdi];
  if (!pde.present) {
    return;
  }

  const paddr_t pt_phys = frame_to_phys(pde.frame);
  auto* pt = phys_to_virt(pt_phys).ptr<PageTable>();
  PageEntry& pte = pt->entry[pt_index(virt)];
  if (!pte.present) {
    return;
  }

  memset(&pte, 0, sizeof(pte));
  asm volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void sync_kernel_mappings(PageTable* pd) {
  memcpy(&pd->entry[kKernelPdeStart], &boot_page_directory.entry[kKernelPdeStart],
         (PAGES_PER_TABLE - kKernelPdeStart) * sizeof(PageEntry));
}

void load(paddr_t pd_phys) { asm volatile("mov %0, %%cr3" ::"r"(pd_phys) : "memory"); }

bool is_user_mapped(const PageTable* pd, vaddr_t va, bool writeable) {
  const PageEntry& pde = pd->entry[pd_index(va)];
  if (!pde.present || !pde.user) {
    return false;
  }
  const auto* pt = phys_to_virt(frame_to_phys(pde.frame)).ptr<const PageTable>();
  const PageEntry& pte = pt->entry[pt_index(va)];
  if (!pte.present || !pte.user) {
    return false;
  }
  if (writeable && !pte.rw) {
    return false;
  }
  return true;
}

PageDir copy(const PageTable* src_pd) {
  auto [new_phys, new_pd] = create();

  for (uint32_t pdi = 0; pdi < kKernelPdeStart; ++pdi) {
    const PageEntry& pde = src_pd->entry[pdi];
    if (!pde.present) {
      continue;
    }

    const auto* src_pt = phys_to_virt(frame_to_phys(pde.frame)).ptr<const PageTable>();

    for (uint32_t pti = 0; pti < PAGES_PER_TABLE; ++pti) {
      const PageEntry& pte = src_pt->entry[pti];
      if (!pte.present) {
        continue;
      }

      paddr_t new_page = kPmm.alloc();
      assert(new_page && "AddressSpace::copy(): out of physical memory\n");

      const auto* src = phys_to_virt(frame_to_phys(pte.frame)).ptr<const uint8_t>();
      auto* dst = phys_to_virt(new_page).ptr<uint8_t>();
      memcpy(dst, src, PAGE_SIZE);

      const vaddr_t va{(pdi << 22u) | (pti << PAGE_OFFSET_BITS)};
      map(new_pd, va, new_page, pte.rw != 0, pte.user != 0);
    }
  }

  return {new_phys, new_pd};
}

void destroy(PageTable* pd, paddr_t pd_phys) {
  // Free all user-space page tables and their mapped pages.
  for (uint32_t pdi = 0; pdi < kKernelPdeStart; ++pdi) {
    PageEntry& pde = pd->entry[pdi];
    if (!pde.present) {
      continue;
    }

    const paddr_t pt_phys = frame_to_phys(pde.frame);
    auto* pt = phys_to_virt(pt_phys).ptr<PageTable>();

    for (uint32_t pti = 0; pti < PAGES_PER_TABLE; ++pti) {
      PageEntry& pte = pt->entry[pti];
      if (pte.present) {
        kPmm.free(frame_to_phys(pte.frame));
      }
    }

    // Free the page table itself.
    kPmm.free(pt_phys);
  }

  kPmm.free(pd_phys);
}

}  // namespace AddressSpace
