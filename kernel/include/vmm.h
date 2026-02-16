#pragma once

#include "paging.h"

/*
 * Virtual Memory Mapper
 *
 * Provides page-granularity mapping and unmapping of virtual addresses to
 * physical frames. Operates on the single kernel page directory
 * (boot_page_directory) that is set up by boot.S and extended on demand.
 *
 * All virtual addresses must be 4 KiB-aligned. Physical addresses returned
 * by kPmm.alloc() are always 4 KiB-aligned, so they satisfy this requirement.
 *
 * Newly required page tables are allocated from the PMM. Their physical
 * addresses must fall within the first 8 MiB so that they can be reached via
 * phys_to_virt(). The PMM allocates from the lowest available frames first,
 * so this holds in practice; a panic fires if it is ever violated.
 */

namespace VMM {

    // Map the virtual page at `virt` to the physical page at `phys`.
    // - Both addresses must be PAGE_SIZE-aligned.
    // - writeable: if true the page is writable from ring-0 (and ring-3 when
    // user).
    // - user: if true the page is also accessible from ring-3.
    // Remapping an already-mapped virtual page is allowed.
    void map(vaddr_t virt, paddr_t phys, bool writeable = true,
             bool user = false);

    // Remove the mapping for the virtual page at `virt`.
    // No-op if the address is not currently mapped.
    // The caller is responsible for freeing the backing physical frame.
    void unmap(vaddr_t virt);

    // Translate `virt` to its backing physical address.
    // Returns 0 if `virt` is not mapped.
    // The low 12 bits of `virt` (the page offset) are preserved in the result.
    [[nodiscard]]
    paddr_t get_phys(vaddr_t virt);

}  // namespace VMM
