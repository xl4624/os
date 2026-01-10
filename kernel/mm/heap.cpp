#include "heap.hpp"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "panic.h"
#include "x86.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  Block header  (must be exactly 16 bytes on i686 so every payload is
//  16-byte aligned when the heap base is 16-byte aligned)
// ─────────────────────────────────────────────────────────────────────────────
struct BlockHeader {
    uint32_t     size;   // bytes of payload (not counting this header)
    uint32_t     free;   // 1 = free, 0 = allocated
    BlockHeader *next;   // reserved for future explicit free-list link
    BlockHeader *prev;   // reserved for future explicit free-list link
};

static_assert(sizeof(BlockHeader) == 16,
              "BlockHeader must be 16 bytes for payload alignment invariant");

// ─────────────────────────────────────────────────────────────────────────────
//  Module-private state
// ─────────────────────────────────────────────────────────────────────────────
static BlockHeader *heap_base_ = nullptr;   // pointer to first block
static uint8_t     *heap_end_  = nullptr;   // one byte past the last heap byte

// ─────────────────────────────────────────────────────────────────────────────
//  Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

// Round sz up to the nearest multiple of 16; minimum 16.
static inline size_t align16(size_t sz) {
    if (sz == 0) return 16;
    return (sz + 15u) & ~static_cast<size_t>(15u);
}

// Pointer to the block immediately following hdr in address order.
static inline BlockHeader *block_after(const BlockHeader *hdr) {
    return reinterpret_cast<BlockHeader *>(
        reinterpret_cast<uint8_t *>(const_cast<BlockHeader *>(hdr))
        + sizeof(BlockHeader) + hdr->size);
}

// Linear scan from heap_base_: return the block whose physical successor is
// 'target', or nullptr if 'target' is the first block.
static BlockHeader *find_prev_block(const BlockHeader *target) {
    if (target == heap_base_) return nullptr;
    BlockHeader *cur = heap_base_;
    while (reinterpret_cast<uint8_t *>(cur) < heap_end_) {
        BlockHeader *nxt = block_after(cur);
        if (nxt == target) return cur;
        cur = nxt;
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Heap::init
// ─────────────────────────────────────────────────────────────────────────────
namespace Heap {

void init() {
    // &kernel_end is the virtual address of the end of the kernel image
    // (4K-aligned by the linker script).  Round up to 16 defensively.
    uintptr_t base = reinterpret_cast<uintptr_t>(&kernel_end);
    base = (base + 15u) & ~static_cast<uintptr_t>(15u);

    heap_base_ = reinterpret_cast<BlockHeader *>(base);
    heap_end_  = reinterpret_cast<uint8_t *>(base + HEAP_SIZE);

    // The heap must fit inside the 8 MiB boot-mapped window.
    constexpr uintptr_t MAPPED_END = 0xC0000000u + 8u * 1024u * 1024u;
    if (reinterpret_cast<uintptr_t>(heap_end_) > MAPPED_END) {
        panic("Heap::init: heap end %p exceeds mapped region %p\n",
              reinterpret_cast<void *>(heap_end_),
              reinterpret_cast<void *>(MAPPED_END));
    }

    // One initial free block covering the entire heap payload area.
    heap_base_->size = static_cast<uint32_t>(HEAP_SIZE - sizeof(BlockHeader));
    heap_base_->free = 1;
    heap_base_->next = nullptr;
    heap_base_->prev = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Heap::dump
// ─────────────────────────────────────────────────────────────────────────────
void dump() {
    printf("Heap dump [%p .. %p] (%u KiB):\n",
           reinterpret_cast<void *>(heap_base_),
           reinterpret_cast<void *>(heap_end_),
           static_cast<unsigned>(HEAP_SIZE / 1024));

    size_t total_used  = 0;
    size_t total_free  = 0;
    size_t block_count = 0;

    BlockHeader *cur = heap_base_;
    while (reinterpret_cast<uint8_t *>(cur) < heap_end_) {
        void *payload = reinterpret_cast<uint8_t *>(cur) + sizeof(BlockHeader);
        printf("  [%p] payload=%p size=%6u %s\n",
               reinterpret_cast<void *>(cur),
               payload,
               static_cast<unsigned>(cur->size),
               cur->free ? "FREE" : "USED");
        if (cur->free)
            total_free += cur->size;
        else
            total_used += cur->size;
        ++block_count;
        cur = block_after(cur);
    }

    printf("  blocks=%u  used=%u  free=%u  (payload bytes)\n",
           static_cast<unsigned>(block_count),
           static_cast<unsigned>(total_used),
           static_cast<unsigned>(total_free));
}

}  // namespace Heap

// ─────────────────────────────────────────────────────────────────────────────
//  kmalloc
// ─────────────────────────────────────────────────────────────────────────────
void *kmalloc(size_t size) {
    if (!heap_base_)
        panic("kmalloc: heap not initialised\n");
    if (size == 0)
        return nullptr;

    const size_t need = align16(size);

    // First-fit linear scan through all blocks.
    BlockHeader *cur = heap_base_;
    while (reinterpret_cast<uint8_t *>(cur) < heap_end_) {
        if (cur->free && static_cast<size_t>(cur->size) >= need) {
            // Split only if the remainder can hold a header + ≥ 16 B payload.
            if (static_cast<size_t>(cur->size) >= need + sizeof(BlockHeader) + 16u) {
                auto *rem = reinterpret_cast<BlockHeader *>(
                    reinterpret_cast<uint8_t *>(cur) + sizeof(BlockHeader) + need);
                rem->size = static_cast<uint32_t>(
                    cur->size - need - sizeof(BlockHeader));
                rem->free = 1;
                rem->next = nullptr;
                rem->prev = nullptr;
                cur->size = static_cast<uint32_t>(need);
            }
            cur->free = 0;
            return reinterpret_cast<uint8_t *>(cur) + sizeof(BlockHeader);
        }
        cur = block_after(cur);
    }

    return nullptr;  // OOM
}

// ─────────────────────────────────────────────────────────────────────────────
//  kfree
// ─────────────────────────────────────────────────────────────────────────────
void kfree(void *ptr) {
    if (!ptr) return;

    auto *hdr = reinterpret_cast<BlockHeader *>(
        reinterpret_cast<uint8_t *>(ptr) - sizeof(BlockHeader));

    if (reinterpret_cast<uint8_t *>(hdr) < reinterpret_cast<uint8_t *>(heap_base_)
        || reinterpret_cast<uint8_t *>(ptr) >= heap_end_) {
        panic("kfree(%p): pointer outside heap [%p, %p)\n",
              ptr,
              reinterpret_cast<void *>(heap_base_),
              reinterpret_cast<void *>(heap_end_));
    }

    if (hdr->free)
        panic("kfree(%p): double free\n", ptr);

    hdr->free = 1;

    // Forward coalesce: absorb the next block if it is free.
    BlockHeader *nxt = block_after(hdr);
    if (reinterpret_cast<uint8_t *>(nxt) < heap_end_ && nxt->free) {
        hdr->size += static_cast<uint32_t>(sizeof(BlockHeader)) + nxt->size;
    }

    // Backward coalesce: absorb this block into the previous one if free.
    BlockHeader *prv = find_prev_block(hdr);
    if (prv && prv->free) {
        prv->size += static_cast<uint32_t>(sizeof(BlockHeader)) + hdr->size;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  kcalloc
// ─────────────────────────────────────────────────────────────────────────────
void *kcalloc(size_t nmemb, size_t size) {
    // Overflow-safe total size computation.
    if (nmemb != 0 && size > static_cast<size_t>(-1) / nmemb)
        return nullptr;
    const size_t total = nmemb * size;
    void *ptr = kmalloc(total);
    if (ptr)
        memset(ptr, 0, total);
    return ptr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  krealloc
// ─────────────────────────────────────────────────────────────────────────────
void *krealloc(void *ptr, size_t size) {
    if (!ptr)    return kmalloc(size);
    if (size == 0) { kfree(ptr); return nullptr; }

    auto *hdr = reinterpret_cast<BlockHeader *>(
        reinterpret_cast<uint8_t *>(ptr) - sizeof(BlockHeader));

    // If the current block already covers the request, return as-is.
    if (static_cast<size_t>(hdr->size) >= align16(size))
        return ptr;

    void *new_ptr = kmalloc(size);
    if (!new_ptr)
        return nullptr;  // original ptr preserved (C standard behaviour)

    memcpy(new_ptr, ptr, hdr->size);
    kfree(ptr);
    return new_ptr;
}
