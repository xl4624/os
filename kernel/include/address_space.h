#pragma once

#include "paging.h"

/*
 * Per-process address space management.
 *
 * Each process has its own page directory. Kernel mappings (PDE indices
 * 768–1023, covering 0xC0000000–0xFFFFFFFF) are shared across all processes
 * by copying the PDE entries from boot_page_directory. User mappings (PDE
 * indices 0–767) are per-process.
 *
 * Page directories and page tables must reside in the first 8 MiB of
 * physical memory so they can be accessed via phys_to_virt(). The PMM
 * allocates lowest frames first, satisfying this constraint.
 */

namespace AddressSpace {

    // Kernel PDE range: indices 768–1023 (0xC0000000 / 4 MiB per entry = 768).
    static constexpr uint32_t kKernelPdeStart = KERNEL_VMA >> 22;

    // Allocate a new page directory with kernel mappings copied from
    // boot_page_directory and user entries zeroed.
    // Returns the physical and virtual addresses of the new page directory.
    struct PageDir {
        paddr_t phys;
        PageTable *virt;
    };
    PageDir create();

    // Map a single page in a specific page directory.
    // The page directory does not need to be the currently loaded one.
    void map(PageTable *pd, vaddr_t virt, paddr_t phys, bool writeable,
             bool user);

    // Unmap a single page from a page directory, freeing its physical frame.
    // Invalidates the TLB entry for `virt` via invlpg. No-op if not mapped.
    void unmap(PageTable *pd, vaddr_t virt);

    // Copy current kernel PDE entries (768–1023) from boot_page_directory
    // into the given page directory. Call before switching to this address
    // space to pick up any new kernel mappings (e.g. heap growth).
    void sync_kernel_mappings(PageTable *pd);

    // Load a page directory into CR3, switching the active address space.
    void load(paddr_t pd_phys);

    // Free all user-space pages and page tables owned by this page directory,
    // then free the page directory page itself.
    void destroy(PageTable *pd, paddr_t pd_phys);

}  // namespace AddressSpace
