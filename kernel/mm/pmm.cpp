#include "pmm.hpp"

#include <stdio.h>

#include "panic.h"

Bitmap::Bitmap(void *buffer, size_t size) {
    buffer_ = reinterpret_cast<uint32_t *>(buffer);
    size_ = size;
}

void Bitmap::set(size_t bit) {
    if (bit >= size_) {
        panic("Bitmap::set(%zu) index out of bounds\n", bit);
    }
    buffer_[bit / 32] |= 1u << (bit % 32);
}

void Bitmap::clear(size_t bit) {
    if (bit >= size_) {
        panic("Bitmap::clear(%zu) index out of bounds\n", bit);
    }
    buffer_[bit / 32] &= ~(1u << (bit % 32));
}

bool Bitmap::is_set(size_t bit) const {
    if (bit >= size_) {
        panic("Bitmap::is_set(%zu) index out of bounds\n", bit);
    }
    return (buffer_[bit / 32] & (1u << (bit % 32))) != 0;
}

void Bitmap::print() const {
    printf("Bitmap state (%zu total bits):\n", size_);

    for (size_t i = 0; i < size_; i++) {
        printf("%c", is_set(i) ? '1' : '0');

        constexpr size_t BITS_PER_BYTE = 8;
        if ((i + 1) % BITS_PER_BYTE == 0)
            printf(" ");

        if ((i + 1) % 32 == 0) {
            printf("\n");
        }
    }

    if (size_ % 32 != 0) {
        printf("\n");
    }
}
