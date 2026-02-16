#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "paging.h"
#include "panic.h"

// Fixed-size bitmap with inline storage.
// N must be a positive multiple of 32.
template <size_t N>
class Bitmap {
    static_assert(N > 0 && N % 32 == 0,
                  "Bitmap size must be a positive multiple of 32");

   public:
    Bitmap() = default;

    // Mark bit as used.
    void set(size_t bit) {
        check_bounds(bit, "set");
        words_[word_of(bit)] |= mask_of(bit);
    }

    // Mark bit as free.
    void clear(size_t bit) {
        check_bounds(bit, "clear");
        words_[word_of(bit)] &= ~mask_of(bit);
    }

    [[nodiscard]]
    bool is_set(size_t bit) const {
        check_bounds(bit, "is_set");
        return (words_[word_of(bit)] & mask_of(bit)) != 0;
    }

    // Set all N bits (mark everything as used).
    void fill() {
        for (auto &w : words_) {
            w = ~uint32_t{0};
        }
    }

    // Returns the index of the first clear bit, or N if all bits are set.
    [[nodiscard]]
    size_t find_first_clear() const {
        for (size_t w = 0; w < WORD_COUNT; ++w) {
            if (words_[w] != ~uint32_t{0}) {
                return w * BITS_PER_WORD
                       + static_cast<size_t>(__builtin_ctz(~words_[w]));
            }
        }
        return N;
    }

    // Print up to `count` bits starting at `from`.
    void print_range(size_t from, size_t count) const {
        const size_t end = from + count < N ? from + count : N;
        printf("Bitmap [%zu..%zu):\n", from, end);
        for (size_t i = from; i < end; ++i) {
            printf("%c", is_set(i) ? '1' : '0');
            if ((i - from + 1) % 8 == 0) {
                printf(" ");
            }
            if ((i - from + 1) % 64 == 0) {
                printf("\n");
            }
        }
        if ((end - from) % 64 != 0)
            printf("\n");
    }

    static constexpr size_t SIZE = N;

   private:
    static constexpr size_t BITS_PER_WORD = 32;
    static constexpr size_t WORD_COUNT = N / BITS_PER_WORD;

    [[nodiscard]]
    static constexpr size_t word_of(size_t bit) {
        return bit / BITS_PER_WORD;
    }

    [[nodiscard]]
    static constexpr uint32_t mask_of(size_t bit) {
        return uint32_t{1} << (bit % BITS_PER_WORD);
    }

    void check_bounds(size_t bit, const char *method) const {
        if (bit >= N) {
            panic("Bitmap::%s(%zu): index out of bounds (size=%zu)\n", method,
                  bit, N);
        }
    }

    uint32_t words_[WORD_COUNT]{};
};

class PhysicalMemoryManager {
   public:
    PhysicalMemoryManager();
    ~PhysicalMemoryManager() = default;

    // Returns the physical address of a free page frame, or 0 if out of memory.
    [[nodiscard]]
    paddr_t alloc();

    // Releases a previously allocated page frame.
    void free(paddr_t addr);

    [[nodiscard]]
    size_t get_free_count() const {
        return free_count_;
    }

    [[nodiscard]]
    size_t get_used_count() const {
        return total_frames_ - free_count_;
    }

    // Print up to `count` frame bits (1 = used, 0 = free) starting at `from`
    void print_range(size_t from, size_t count) const {
        bitmap_.print_range(from,
                            count < total_frames_ ? count : total_frames_);
    }

   private:
    // 1 bit per 4 KiB frame over full 4 GiB address space = 128 KiB bitmap.
    static constexpr size_t MAX_FRAMES = 1024 * 1024;

    // Marks all frames in [start, start+length) as used
    void mark_used_range(paddr_t start, size_t length);

    // Marks all fully-contained frames in [start, start+length) as free.
    void mark_free_range(paddr_t start, size_t length);

    Bitmap<MAX_FRAMES> bitmap_;
    size_t total_frames_ = 0;
    size_t free_count_ = 0;
};

extern PhysicalMemoryManager kPmm;
