#include "paging.hpp"

#include <stdio.h>
#include <string.h>

PageTable page_directory __attribute__((section(".data")))
__attribute__((aligned(PAGE_SIZE)));
PageTable page_tables[PAGES_PER_TABLE] __attribute__((section(".data")))
__attribute__((aligned(PAGE_SIZE)));

static void paging_enable(uintptr_t pd_addr) {
    asm volatile(
        "mov %0, %%eax\n"
        "mov %%eax, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n" ::"r"(pd_addr)
        : "eax");
}

void paging_init() {
    memset(&page_directory, 0, sizeof(PageTable));
    memset(page_tables, 0, sizeof(PageTable) * PAGES_PER_TABLE);

    for (size_t i = 0; i < PAGES_PER_TABLE; i++) {
        page_tables[0].entry[i] = PageEntry(i * PAGE_SIZE, true, false);
    }
    for (size_t i = 0; i < PAGES_PER_TABLE; i++) {
        page_tables[1].entry[i] =
            PageEntry((i + 1024) * PAGE_SIZE, true, false);
    }
    page_directory.entry[0] =
        PageEntry(reinterpret_cast<paddr_t>(&page_tables[0]), true, false);
    page_directory.entry[1] =
        PageEntry(reinterpret_cast<paddr_t>(&page_tables[1]), true, false);

    paging_enable((uintptr_t)&page_directory);
}
