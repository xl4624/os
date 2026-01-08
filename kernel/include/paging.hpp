#pragma once

#include <stdint.h>

// clang-format off
constexpr uint32_t PAGE_OFFSET_BITS = 12;
constexpr uint32_t PAGE_SIZE = 1U << PAGE_OFFSET_BITS;
constexpr uint32_t PAGE_TABLE_BITS = 10;
constexpr uint32_t PAGES_PER_TABLE = 1U << PAGE_TABLE_BITS;
// clang-format on

using paddr_t = uintptr_t;
using vaddr_t = uintptr_t;

constexpr vaddr_t KERNEL_VMA = 0xC0000000;

[[nodiscard]]
constexpr paddr_t virt_to_phys(vaddr_t vaddr) {
    return vaddr - KERNEL_VMA;
}

[[nodiscard]]
constexpr vaddr_t phys_to_virt(paddr_t paddr) {
    return paddr + KERNEL_VMA;
}

struct PageEntry {
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t writeThrough : 1;
    uint32_t cacheDisabled : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t pat : 1;  // PAT if page table entry, PS (page size) if page
                       // directory entry. Always 0 in our implementation.
    uint32_t global : 1;
    uint32_t available : 3;
    uint32_t frame : 20;

    PageEntry() = default;
    PageEntry(paddr_t phys_addr, bool is_writeable = true, bool is_user = true)
        : present(1),
          rw(is_writeable ? 1 : 0),
          user(is_user ? 1 : 0),
          writeThrough(1),
          cacheDisabled(1),
          accessed(0),
          dirty(0),
          pat(0),
          global(0),
          available(0),
          frame(phys_addr >> PAGE_OFFSET_BITS)  // lower 16 bits used for flags
    {}
} __attribute__((packed));

struct PageTable {
    PageEntry entry[PAGES_PER_TABLE];
} __attribute__((aligned(PAGE_SIZE)));

extern "C" PageTable boot_page_directory;
