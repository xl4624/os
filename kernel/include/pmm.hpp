#pragma once

#include <stddef.h>
#include <stdint.h>

class Bitmap {
   public:
    Bitmap(void *buffer, size_t bits);
    ~Bitmap() = default;
    void set(size_t bit);
    void clear(size_t bit);
    [[nodiscard]] bool is_set(size_t bit) const;
    void print() const;

   private:
    uint32_t *buffer_ = nullptr;
    size_t size_ = 0;
};
