#include "vmm.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "paging.h"
#include "panic.h"
#include "pmm.h"

// =============================================================================
// Internal helpers
// =============================================================================

// Top 10 bits of a virtual address -> page directory index.
static constexpr uint32_t pd_index(vaddr_t va) {
    return va >> (PAGE_TABLE_BITS + PAGE_OFFSET_BITS);
}

// Middle 10 bits of a virtual address -> page table index.
static constexpr uint32_t pt_index(vaddr_t va) {
    return (va >> PAGE_OFFSET_BITS) & (PAGES_PER_TABLE - 1);
}

// Return a virtual pointer to the page table covering `virt`.
// If the PDE is absent and `create` is true, a new page table is allocated
// from the PMM, zeroed, and installed in the page directory.
// Returns nullptr if the PDE is absent and `create` is false.
static PageTable *page_table_for(vaddr_t virt, bool create, bool user) {
    const uint32_t pdi = pd_index(virt);
    PageEntry &pde = boot_page_directory.entry[pdi];

    if (!pde.present) {
        if (!create) {
            return nullptr;
        }

        const paddr_t pt_phys = kPmm.alloc();
        if (!pt_phys) {
            panic("VMM: out of physical memory allocating page table\n");
        }

        // page_table_for is called only after kernel_init() has run, so the
        // first 8 MiB are mapped. PMM hands out the lowest available frames
        // first, which are always in the first 8 MiB.
        static constexpr paddr_t MAPPED_PHYS_END = 8u * 1024u * 1024u;
        if (pt_phys >= MAPPED_PHYS_END) {
            panic("VMM: new page table phys 0x%08x outside mapped region\n",
                  static_cast<unsigned>(pt_phys));
        }

        // Zero the page table before installing it.
        auto *pt = reinterpret_cast<PageTable *>(phys_to_virt(pt_phys));
        memset(pt, 0, sizeof(PageTable));

        // Install in page directory. Use the caller's `user` flag so kernel
        // PDEs stay ring-0 only.
        pde = PageEntry(pt_phys, /*writeable=*/true, /*user=*/user);
        return pt;
    }

    const paddr_t pt_phys = static_cast<paddr_t>(pde.frame) << PAGE_OFFSET_BITS;
    return reinterpret_cast<PageTable *>(phys_to_virt(pt_phys));
}

// =============================================================================
// Public API
// =============================================================================

namespace VMM {

    void map(vaddr_t virt, paddr_t phys, bool writeable, bool user) {
        assert(!(virt & (PAGE_SIZE - 1))
               && "VMM::map(): virt address is not page-aligned");
        assert(!(phys & (PAGE_SIZE - 1))
               && "VMM::map(): phys address is not page-aligned");

        PageTable *pt = page_table_for(virt, /*create=*/true, user);
        pt->entry[pt_index(virt)] = PageEntry(phys, writeable, user);

        // Flush the TLB entry for this virtual address.
        asm volatile("invlpg (%0)" ::"r"(virt) : "memory");
    }

    void unmap(vaddr_t virt) {
        assert(!(virt & (PAGE_SIZE - 1))
               && "VMM::unmap(): virt address is not page-aligned");

        PageTable *pt = page_table_for(virt, /*create=*/false, false);
        if (!pt) {
            return;
        }

        PageEntry &pte = pt->entry[pt_index(virt)];
        if (!pte.present) {
            return;
        }

        // Zero the page table entry to unmap it.
        pte = PageEntry{};

        // Flush the TLB entry for this virtual address.
        asm volatile("invlpg (%0)" ::"r"(virt) : "memory");
    }

    paddr_t get_phys(vaddr_t virt) {
        const PageTable *pt = page_table_for(virt, /*create=*/false, false);
        if (!pt) {
            return 0;
        }

        const PageEntry &pte = pt->entry[pt_index(virt)];
        if (!pte.present) {
            return 0;
        }

        return (static_cast<paddr_t>(pte.frame) << PAGE_OFFSET_BITS)
               | (virt & (PAGE_SIZE - 1));
    }

}  // namespace VMM
