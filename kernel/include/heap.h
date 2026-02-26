#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

struct BlockHeader;

/*
 * Kernel heap allocator.
 *
 * Backed by physical pages allocated from the PMM and mapped on demand via
 * VMM::map(). The heap occupies a dedicated virtual region starting at
 * kHeapVirtBase and can grow up to kHeapMaxSize by mapping additional pages.
 */
class Heap {
 public:
  static constexpr uintptr_t kVirtBase = 0xD0000000;
  static constexpr size_t kMaxSize = 16u * 1024u * 1024u;  // 16 MiB cap
  static constexpr size_t kInitialPages = 16;              // 64 KiB

  // Initialise the heap. Allocates kInitialPages physical pages from
  // the PMM and maps them at kVirtBase. Must be called once before any
  // alloc/free, and after the PMM is ready.
  void init();

  [[nodiscard]] void* alloc(size_t size);
  void free(void* ptr);
  [[nodiscard]] void* calloc(size_t nmemb, size_t size);
  [[nodiscard]] void* realloc(void* ptr, size_t size);

  // Print a block-by-block dump of the heap to the VGA terminal (debug).
  void dump() const;

  // Current mapped size in bytes.
  size_t mapped_size() const;

 private:
  BlockHeader* find_last_block() const;
  bool grow(size_t min_bytes);

  BlockHeader* base_ = nullptr;
  uint8_t* end_ = nullptr;  // one byte past the last mapped byte
};

extern Heap kHeap;

__BEGIN_DECLS

[[nodiscard]] void* kmalloc(size_t size);
void kfree(void* ptr);
[[nodiscard]] void* kcalloc(size_t nmemb, size_t size);
[[nodiscard]] void* krealloc(void* ptr, size_t size);

__END_DECLS
