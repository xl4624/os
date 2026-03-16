#include "vmm.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "paging.h"
#include "panic.h"
#include "pmm.h"

namespace {

// Top 10 bits of a virtual address -> page directory index.
constexpr uint32_t pd_index(vaddr_t va) { return va >> (PAGE_TABLE_BITS + PAGE_OFFSET_BITS); }

// Middle 10 bits of a virtual address -> page table index.
constexpr uint32_t pt_index(vaddr_t va) { return (va >> PAGE_OFFSET_BITS) & (PAGES_PER_TABLE - 1); }

// Return a virtual pointer to the page table covering `virt`.
// If the PDE is absent and `create` is true, a new page table is allocated
// from the PMM, zeroed, and installed in the page directory.
// Returns nullptr if the PDE is absent and `create` is false.
PageTable* page_table_for(vaddr_t virt, bool create, bool user) {
  const uint32_t pdi = pd_index(virt);
  PageEntry& pde = boot_page_directory.entry[pdi];

  if (!pde.present) {
    if (!create) {
      return nullptr;
    }

    const paddr_t pt_phys = kPmm.alloc();
    assert(pt_phys && "VMM: out of physical memory allocating page table\n");

    // PMM hands out the lowest available frames first. After
    // map_all_physical_ram() all managed physical memory is accessible
    // via phys_to_virt(), so any frame up to total RAM is safe.
    const paddr_t mapped_phys_end = paddr_t{kPmm.get_total_frames()} * PAGE_SIZE;
    if (pt_phys >= mapped_phys_end) {
      panic("VMM: new page table phys 0x%08x outside mapped region\n",
            static_cast<unsigned>(pt_phys));
    }

    // Zero the page table before installing it.
    auto* pt = phys_to_virt(pt_phys).ptr<PageTable>();
    memset(pt, 0, sizeof(PageTable));

    pde = PageEntry(pt_phys, /*is_writeable=*/true, /*is_user=*/user);
    return pt;
  }

  const paddr_t pt_phys = paddr_t{pde.frame} << PAGE_OFFSET_BITS;
  return phys_to_virt(pt_phys).ptr<PageTable>();
}

}  // namespace

namespace VMM {

void map(vaddr_t virt, paddr_t phys, bool writeable, bool user) {
  assert(!(virt & (PAGE_SIZE - 1)) && "VMM::map(): virt address is not page-aligned");
  assert(!(phys & (PAGE_SIZE - 1)) && "VMM::map(): phys address is not page-aligned");

  PageTable* pt = page_table_for(virt, /*create=*/true, user);
  pt->entry[pt_index(virt)] = PageEntry(phys, writeable, user);

  __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void unmap(vaddr_t virt) {
  assert(!(virt & (PAGE_SIZE - 1)) && "VMM::unmap(): virt address is not page-aligned");

  PageTable* pt = page_table_for(virt, /*create=*/false, false);
  if (pt == nullptr) {
    return;
  }

  PageEntry& pte = pt->entry[pt_index(virt)];
  if (!pte.present) {
    return;
  }

  pte = PageEntry{};

  __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

paddr_t get_phys(vaddr_t virt) {
  const PageTable* pt = page_table_for(virt, /*create=*/false, false);
  if (pt == nullptr) {
    return 0;
  }

  const PageEntry& pte = pt->entry[pt_index(virt)];
  if (!pte.present) {
    return 0;
  }

  return (paddr_t{pte.frame} << PAGE_OFFSET_BITS) | (virt & (PAGE_SIZE - 1));
}

void map_all_physical_ram() {
  constexpr uint32_t kBootMappedEnd = 8U * 1024U * 1024U;
  const uint32_t total_bytes = static_cast<uint32_t>(kPmm.get_total_frames()) * PAGE_SIZE;

  for (uint32_t off = kBootMappedEnd; off < total_bytes; off += PAGE_SIZE) {
    const paddr_t phys{off};
    VMM::map(phys_to_virt(phys), phys, /*writeable=*/true, /*user=*/false);
  }
  VMM::flush_tlb();
}

void flush_tlb() {
  uint32_t cr3;
  __asm__ volatile(
      "mov %%cr3, %0\n"
      "mov %0, %%cr3\n"
      : "=r"(cr3)
      :
      : "memory");
}

}  // namespace VMM
