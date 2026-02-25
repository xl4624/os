#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if defined(__is_libk)

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);
extern void* kcalloc(size_t nmemb, size_t size);
extern void* krealloc(void* ptr, size_t size);

void* malloc(size_t size) { return kmalloc(size); }

void free(void* ptr) { kfree(ptr); }

void* calloc(size_t nmemb, size_t size) { return kcalloc(nmemb, size); }

void* realloc(void* ptr, size_t size) { return krealloc(ptr, size); }

#elif defined(__is_libc)

#include <unistd.h>

/*
 * Userspace first-fit heap allocator backed by sbrk().
 *
 * Block layout:  [BlockHeader | payload ...]
 * Minimum allocation granularity: 16 bytes (for alignment).
 */

struct BlockHeader {
  size_t size; /* payload bytes (not counting this header) */
  int free;
};

#define HEADER_SIZE sizeof(struct BlockHeader)
#define ALIGN 16u
#define MIN_SPLIT (HEADER_SIZE + ALIGN)

static struct BlockHeader* heap_base = NULL;

/* Round up to nearest multiple of ALIGN, minimum ALIGN. */
static size_t align_up(size_t sz) {
  if (sz == 0) return ALIGN;
  return (sz + ALIGN - 1) & ~(size_t)(ALIGN - 1);
}

/* Return pointer to the block immediately after hdr in memory. */
static struct BlockHeader* block_after(struct BlockHeader* hdr) {
  return (struct BlockHeader*)((char*)hdr + HEADER_SIZE + hdr->size);
}

void* malloc(size_t size) {
  if (size == 0) return NULL;

  size_t need = align_up(size);

  /* First-fit search. */
  struct BlockHeader* cur = heap_base;
  while (cur != NULL) {
    struct BlockHeader* next = block_after(cur);
    /* Check if next is still within our heap (i.e. not past sbrk(0)). */
    if ((char*)next > (char*)sbrk(0)) break;

    if (cur->free && cur->size >= need) {
      /* Split if there is enough leftover for a new block. */
      if (cur->size >= need + MIN_SPLIT) {
        struct BlockHeader* rem = (struct BlockHeader*)((char*)cur + HEADER_SIZE + need);
        rem->size = cur->size - need - HEADER_SIZE;
        rem->free = 1;
        cur->size = need;
      }
      cur->free = 0;
      return (char*)cur + HEADER_SIZE;
    }

    cur = next;
  }

  /* No suitable block found; extend heap via sbrk(). */
  size_t request = need + HEADER_SIZE;
  void* block = sbrk((int)request);
  if (block == (void*)-1) return NULL;

  struct BlockHeader* hdr = (struct BlockHeader*)block;
  hdr->size = need;
  hdr->free = 0;

  if (heap_base == NULL) heap_base = hdr;

  return (char*)hdr + HEADER_SIZE;
}

void free(void* ptr) {
  if (!ptr) return;

  struct BlockHeader* hdr = (struct BlockHeader*)((char*)ptr - HEADER_SIZE);
  hdr->free = 1;

  /* Coalesce with the next block if it is free. */
  struct BlockHeader* next = block_after(hdr);
  if ((char*)next < (char*)sbrk(0) && next->free) {
    hdr->size += HEADER_SIZE + next->size;
  }

  /* Coalesce with the previous block if it is free. */
  if (hdr != heap_base) {
    struct BlockHeader* prev = heap_base;
    while (prev != NULL) {
      struct BlockHeader* pnext = block_after(prev);
      if ((char*)pnext >= (char*)sbrk(0)) break;
      if (pnext == hdr) {
        if (prev->free) {
          prev->size += HEADER_SIZE + hdr->size;
        }
        return;
      }
      prev = pnext;
    }
  }
}

void* calloc(size_t nmemb, size_t size) {
  if (nmemb != 0 && size > (size_t)-1 / nmemb) return NULL;
  size_t total = nmemb * size;
  void* ptr = malloc(total);
  if (ptr) memset(ptr, 0, total);
  return ptr;
}

void* realloc(void* ptr, size_t size) {
  if (!ptr) return malloc(size);
  if (size == 0) {
    free(ptr);
    return NULL;
  }

  struct BlockHeader* hdr = (struct BlockHeader*)((char*)ptr - HEADER_SIZE);
  size_t need = align_up(size);

  if (hdr->size >= need) return ptr;

  void* new_ptr = malloc(size);
  if (!new_ptr) return NULL;

  memcpy(new_ptr, ptr, hdr->size);
  free(ptr);
  return new_ptr;
}

#endif
