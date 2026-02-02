#include "heap.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "paging.h"
#include "panic.h"
#include "pmm.h"
#include "vmm.h"

struct BlockHeader {
    uint32_t size;  // bytes of payload (not counting this header)
    bool free;
    BlockHeader *next;
    BlockHeader *prev;
};

static_assert(sizeof(BlockHeader) == 16,
              "BlockHeader must be 16 bytes for payload alignment invariant");

// Round sz up to the nearest multiple of 16; minimum 16.
static inline size_t align16(size_t sz) {
    if (sz == 0) {
        return 16;
    }
    return (sz + 15u) & ~static_cast<size_t>(15u);
}

// Pointer to the block immediately following hdr in address order.
static inline BlockHeader *block_after(const BlockHeader *hdr) {
    return reinterpret_cast<BlockHeader *>(
        reinterpret_cast<uint8_t *>(const_cast<BlockHeader *>(hdr))
        + sizeof(BlockHeader) + hdr->size);
}

// Linear scan: return the block whose successor is 'target', or nullptr.
static BlockHeader *find_prev_block(const BlockHeader *base, const uint8_t *end,
                                    const BlockHeader *target) {
    assert(base != nullptr && "find_prev_block(): base is null");
    if (target == base) {
        return nullptr;
    }
    BlockHeader *cur = const_cast<BlockHeader *>(base);
    while (reinterpret_cast<uint8_t *>(cur) < end) {
        BlockHeader *nxt = block_after(cur);
        if (nxt == target) {
            return cur;
        }
        cur = nxt;
    }
    return nullptr;
}

Heap kHeap;

BlockHeader *Heap::find_last_block() const {
    BlockHeader *last = base_;
    while (true) {
        BlockHeader *nxt = block_after(last);
        if (reinterpret_cast<uint8_t *>(nxt) >= end_) {
            return last;
        }
        last = nxt;
    }
}

bool Heap::grow(size_t min_bytes) {
    const size_t pages_needed = (min_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    const uintptr_t limit = kVirtBase + kMaxSize;
    uintptr_t va = reinterpret_cast<uintptr_t>(end_);

    // Refuse to grow if it would exceed the heap's maximum virtual address
    // limit.
    if (va + pages_needed * PAGE_SIZE > limit) {
        return false;
    }

    // Find the last block BEFORE advancing end_, so the scan doesn't
    // walk into the freshly mapped (uninitialised) pages.
    uint8_t *old_end = end_;
    BlockHeader *last = find_last_block();

    for (size_t i = 0; i < pages_needed; ++i) {
        paddr_t phys = kPmm.alloc();
        if (!phys) {
            panic("Heap::grow: out of physical memory\n");
        }

        VMM::map(static_cast<vaddr_t>(va), phys);
        va += PAGE_SIZE;
    }

    const size_t added = pages_needed * PAGE_SIZE;
    end_ = reinterpret_cast<uint8_t *>(va);

    // Extend the last block if it is free, otherwise create a new free block.
    if (reinterpret_cast<uint8_t *>(block_after(last)) == old_end
        && last->free) {
        last->size += static_cast<uint32_t>(added);
    } else {
        auto *blk = reinterpret_cast<BlockHeader *>(old_end);
        blk->size = static_cast<uint32_t>(added - sizeof(BlockHeader));
        blk->free = true;
        blk->next = nullptr;
        blk->prev = nullptr;
    }

    return true;
}

size_t Heap::mapped_size() const {
    return static_cast<size_t>(end_ - reinterpret_cast<uint8_t *>(base_));
}

void Heap::init() {
    assert(base_ == nullptr && "Heap::init(): called more than once");

    base_ = reinterpret_cast<BlockHeader *>(kVirtBase);
    end_ = reinterpret_cast<uint8_t *>(kVirtBase);

    for (size_t i = 0; i < kInitialPages; ++i) {
        paddr_t phys = kPmm.alloc();
        if (!phys) {
            panic("Heap::init: out of physical memory\n");
        }
        vaddr_t va = kVirtBase + i * PAGE_SIZE;
        VMM::map(va, phys);
        end_ += PAGE_SIZE;
    }

    const size_t initial_size = kInitialPages * PAGE_SIZE;

    base_->size = static_cast<uint32_t>(initial_size - sizeof(BlockHeader));
    base_->free = true;
    base_->next = nullptr;
    base_->prev = nullptr;
}

void Heap::dump() const {
    assert(base_ != nullptr && "Heap::dump(): called before Heap::init()");
    printf("Heap dump [%p .. %p] (%u KiB):\n",
           reinterpret_cast<const void *>(base_),
           reinterpret_cast<const void *>(end_),
           static_cast<unsigned>(mapped_size() / 1024));

    size_t total_used = 0;
    size_t total_free = 0;
    size_t block_count = 0;

    BlockHeader *cur = base_;
    while (reinterpret_cast<uint8_t *>(cur) < end_) {
        void *payload = reinterpret_cast<uint8_t *>(cur) + sizeof(BlockHeader);
        printf("  [%p] payload=%p size=%6u %s\n", reinterpret_cast<void *>(cur),
               payload, static_cast<unsigned>(cur->size),
               cur->free ? "FREE" : "USED");
        if (cur->free) {
            total_free += cur->size;
        } else {
            total_used += cur->size;
        }
        ++block_count;
        cur = block_after(cur);
    }

    printf("  blocks=%u  used=%u  free=%u  (payload bytes)\n",
           static_cast<unsigned>(block_count),
           static_cast<unsigned>(total_used),
           static_cast<unsigned>(total_free));
}

void *Heap::alloc(size_t size) {
    if (!base_) {
        panic("kmalloc: heap not initialised\n");
    }
    if (size == 0) {
        return nullptr;
    }

    const size_t need = align16(size);

    BlockHeader *cur = base_;
    while (reinterpret_cast<uint8_t *>(cur) < end_) {
        if (cur->free && static_cast<size_t>(cur->size) >= need) {
            if (static_cast<size_t>(cur->size)
                >= need + sizeof(BlockHeader) + 16u) {
                auto *rem = reinterpret_cast<BlockHeader *>(
                    reinterpret_cast<uint8_t *>(cur) + sizeof(BlockHeader)
                    + need);
                rem->size = static_cast<uint32_t>(cur->size - need
                                                  - sizeof(BlockHeader));
                rem->free = true;
                rem->next = nullptr;
                rem->prev = nullptr;
                cur->size = static_cast<uint32_t>(need);
            }
            cur->free = false;
            return reinterpret_cast<uint8_t *>(cur) + sizeof(BlockHeader);
        }
        cur = block_after(cur);
    }

    if (!grow(need + sizeof(BlockHeader))) {
        return nullptr;
    }
    return alloc(size);
}

void Heap::free(void *ptr) {
    if (!ptr) {
        return;
    }

    auto *hdr = reinterpret_cast<BlockHeader *>(reinterpret_cast<uint8_t *>(ptr)
                                                - sizeof(BlockHeader));

    if (reinterpret_cast<uint8_t *>(hdr) < reinterpret_cast<uint8_t *>(base_)
        || reinterpret_cast<uint8_t *>(ptr) >= end_) {
        panic("kfree(%p): pointer outside heap [%p, %p)\n", ptr,
              reinterpret_cast<void *>(base_), reinterpret_cast<void *>(end_));
    }

    if (hdr->free) {
        panic("kfree(%p): double free\n", ptr);
    }

    hdr->free = true;

    BlockHeader *nxt = block_after(hdr);
    if (reinterpret_cast<uint8_t *>(nxt) < end_ && nxt->free) {
        hdr->size += static_cast<uint32_t>(sizeof(BlockHeader)) + nxt->size;
    }

    BlockHeader *prv = find_prev_block(base_, end_, hdr);
    if (prv && prv->free) {
        prv->size += static_cast<uint32_t>(sizeof(BlockHeader)) + hdr->size;
    }
}

void *Heap::calloc(size_t nmemb, size_t size) {
    assert(base_ != nullptr && "Heap::calloc(): called before Heap::init()");
    if (nmemb != 0 && size > static_cast<size_t>(-1) / nmemb) {
        return nullptr;
    }
    const size_t total = nmemb * size;
    void *ptr = alloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *Heap::realloc(void *ptr, size_t size) {
    assert(base_ != nullptr && "Heap::realloc(): called before Heap::init()");
    if (!ptr) {
        return alloc(size);
    }
    if (size == 0) {
        free(ptr);
        return nullptr;
    }

    auto *hdr = reinterpret_cast<BlockHeader *>(reinterpret_cast<uint8_t *>(ptr)
                                                - sizeof(BlockHeader));

    assert(reinterpret_cast<uint8_t *>(hdr)
               >= reinterpret_cast<uint8_t *>(base_)
           && reinterpret_cast<uint8_t *>(ptr) < end_
           && "Heap::realloc(): ptr outside heap bounds");

    if (static_cast<size_t>(hdr->size) >= align16(size)) {
        return ptr;
    }

    void *new_ptr = alloc(size);
    if (!new_ptr) {
        return nullptr;
    }

    memcpy(new_ptr, ptr, hdr->size);
    free(ptr);
    return new_ptr;
}

// ==============================================================
// C wrappers
// ==============================================================

void *kmalloc(size_t size) {
    return kHeap.alloc(size);
}
void kfree(void *ptr) {
    kHeap.free(ptr);
}
void *kcalloc(size_t nmemb, size_t size) {
    return kHeap.calloc(nmemb, size);
}
void *krealloc(void *ptr, size_t size) {
    return kHeap.realloc(ptr, size);
}
