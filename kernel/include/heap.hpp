#pragma once

#include <stddef.h>
#include <sys/cdefs.h>

/*
 * Heap lives in the already-mapped virtual window after kernel_end.
 * Boot page tables cover 0xC0000000–0xC07FFFFF (8 MiB); the kernel image is
 * ~200 KiB, leaving ~6 MiB of mapped space.  We take 4 MiB for the heap.
 */
static constexpr size_t HEAP_SIZE = 4u * 1024u * 1024u;  // 4 MiB

namespace Heap {
    // Initialise the heap. Must be called once before any kmalloc/kfree.
    void init();

    // Print a block-by-block dump of the heap to the VGA terminal (debug).
    void dump();
}  // namespace Heap

__BEGIN_DECLS

void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);

__END_DECLS
