#include "pmm.hpp"

#include <stddef.h>

#include "multiboot.h"
#include "paging.hpp"
#include "panic.h"
#include "x86.hpp"

namespace {

    const multiboot_memory_map_t *mmap_next(
        const multiboot_memory_map_t *entry) {
        return reinterpret_cast<const multiboot_memory_map_t *>(
            reinterpret_cast<uintptr_t>(entry) + entry->size
            + sizeof(entry->size));
    }

}  // namespace

PhysicalMemoryManager::PhysicalMemoryManager() {
    bitmap_.fill();  // All frames start as used; available regions freed below.

    const auto *info = reinterpret_cast<const multiboot_info_t *>(
        phys_to_virt(reinterpret_cast<paddr_t>(mboot_info)));

    if (!(info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        panic("PMM: bootloader did not provide a memory map\n");
    }

    const vaddr_t mmap_virt_end =
        phys_to_virt(static_cast<paddr_t>(info->mmap_addr) + info->mmap_length);

    const auto *mmap_base = reinterpret_cast<const multiboot_memory_map_t *>(
        phys_to_virt(static_cast<paddr_t>(info->mmap_addr)));

    // Pass 1: find the highest available address to derive total_frames_.
    uint64_t highest_addr = 0;
    for (auto *e = mmap_base; reinterpret_cast<vaddr_t>(e) < mmap_virt_end;
         e = mmap_next(e)) {
        if (e->type == MULTIBOOT_MEMORY_AVAILABLE) {
            const uint64_t end = e->addr + e->len;
            if (end > highest_addr)
                highest_addr = end;
        }
    }

    const uint64_t frames_64 = highest_addr / PAGE_SIZE;
    total_frames_ = (frames_64 > static_cast<uint64_t>(MAX_FRAMES))
                        ? MAX_FRAMES
                        : static_cast<size_t>(frames_64);

    // Pass 2: mark all AVAILABLE regions as free.
    for (auto *e = mmap_base; reinterpret_cast<vaddr_t>(e) < mmap_virt_end;
         e = mmap_next(e)) {
        if (e->type == MULTIBOOT_MEMORY_AVAILABLE
            && e->addr < static_cast<uint64_t>(total_frames_) * PAGE_SIZE) {
            mark_free_range(static_cast<paddr_t>(e->addr),
                            static_cast<size_t>(e->len));
        }
    }

    // Reserve regions that must never be handed to callers.

    // First 1 MiB: BIOS data, VGA memory, legacy hardware.
    mark_used_range(0, 0x100000);

    // Kernel image (virtual addresses converted to physical).
    const paddr_t kstart =
        virt_to_phys(reinterpret_cast<vaddr_t>(&kernel_start));
    const paddr_t kend = virt_to_phys(reinterpret_cast<vaddr_t>(&kernel_end));
    mark_used_range(kstart, kend - kstart);

    // Boot page directory + two page tables (3 consecutive 4 KiB pages).
    const paddr_t pd_phys =
        virt_to_phys(reinterpret_cast<vaddr_t>(&boot_page_directory));
    mark_used_range(pd_phys, 3 * PAGE_SIZE);

    // Multiboot info struct.
    const paddr_t mbi_phys = reinterpret_cast<paddr_t>(mboot_info);
    mark_used_range(mbi_phys & ~static_cast<paddr_t>(PAGE_SIZE - 1), PAGE_SIZE);

    // Multiboot modules.
    if ((info->flags & MULTIBOOT_INFO_MODS) && info->mods_count > 0) {
        const auto *mods = reinterpret_cast<const multiboot_module_t *>(
            phys_to_virt(static_cast<paddr_t>(info->mods_addr)));
        for (uint32_t i = 0; i < info->mods_count; ++i) {
            const paddr_t mod_start =
                mods[i].mod_start & ~static_cast<paddr_t>(PAGE_SIZE - 1);
            const size_t mod_len = mods[i].mod_end - mod_start;
            mark_used_range(mod_start, mod_len);
        }
    }
}

void PhysicalMemoryManager::mark_free_range(paddr_t start, size_t length) {
    // Round start UP and end DOWN so we only free fully-available frames.
    const size_t first =
        (static_cast<size_t>(start) + PAGE_SIZE - 1) / PAGE_SIZE;
    const size_t last = (static_cast<size_t>(start) + length) / PAGE_SIZE;

    for (size_t i = first; i < last && i < total_frames_; ++i) {
        if (bitmap_.is_set(i)) {
            bitmap_.clear(i);
            ++free_count_;
        }
    }
}

void PhysicalMemoryManager::mark_used_range(paddr_t start, size_t length) {
    // Round start DOWN and end UP so we cover every frame that overlaps the
    // region.
    const size_t first = static_cast<size_t>(start) / PAGE_SIZE;
    const size_t last =
        (static_cast<size_t>(start) + length + PAGE_SIZE - 1) / PAGE_SIZE;

    for (size_t i = first; i < last && i < total_frames_; ++i) {
        if (!bitmap_.is_set(i)) {
            bitmap_.set(i);
            if (free_count_ > 0)
                --free_count_;
        }
    }
}

paddr_t PhysicalMemoryManager::alloc() {
    const size_t frame = bitmap_.find_first_clear();
    if (frame >= total_frames_)
        return 0;  // Out of memory.
    bitmap_.set(frame);
    --free_count_;
    return static_cast<paddr_t>(frame) * PAGE_SIZE;
}

void PhysicalMemoryManager::free(paddr_t addr) {
    const size_t frame = static_cast<size_t>(addr) / PAGE_SIZE;
    if (frame >= total_frames_) {
        panic("PMM::free(%p): address out of range\n",
              reinterpret_cast<void *>(addr));
    }
    if (!bitmap_.is_set(frame)) {
        panic("PMM::free(%p): double free\n", reinterpret_cast<void *>(addr));
    }
    bitmap_.clear(frame);
    ++free_count_;
}

PhysicalMemoryManager pmm;
