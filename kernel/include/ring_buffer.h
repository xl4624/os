#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Fixed-size FIFO queue with O(1) push/pop operations.
 *
 * Elements are stored in a contiguous array. No dynamic allocation.
 */
template <typename T, size_t Capacity>
class RingBuffer {
   public:
    static_assert(Capacity > 0, "RingBuffer capacity must be > 0");

    RingBuffer() = default;
    ~RingBuffer() = default;

    RingBuffer(const RingBuffer &) = delete;
    RingBuffer &operator=(const RingBuffer &) = delete;
    RingBuffer(RingBuffer &&) = delete;
    RingBuffer &operator=(RingBuffer &&) = delete;

    /**
     * Returns true if buffer contains no elements.
     */
    [[nodiscard]]
    bool is_empty() const {
        return count_ == 0;
    }

    /**
     * Returns true if buffer has reached capacity.
     */
    [[nodiscard]]
    bool is_full() const {
        return count_ == Capacity;
    }

    /**
     * Returns current number of stored elements.
     */
    [[nodiscard]]
    size_t size() const {
        return count_;
    }

    /**
     * Returns maximum number of elements buffer can hold.
     */
    [[nodiscard]]
    static constexpr size_t capacity() {
        return Capacity;
    }

    /**
     * Removes all elements. Does not destruct stored objects.
     */
    void clear() {
        head_ = 0;
        tail_ = 0;
        count_ = 0;
    }

    /**
     * Adds element to back of queue.
     * Returns true if successful, false if buffer is full (element not added).
     */
    [[nodiscard]]
    bool push(const T &value) {
        if (is_full()) {
            return false;
        }
        buffer_[tail_] = value;
        tail_ = (tail_ + 1) % Capacity;
        ++count_;
        return true;
    }

    /**
     * Removes and returns front element.
     * Returns true if successful, false if buffer is empty.
     */
    [[nodiscard]]
    bool pop(T &out) {
        if (is_empty()) {
            return false;
        }
        out = buffer_[head_];
        head_ = (head_ + 1) % Capacity;
        --count_;
        return true;
    }

    /**
     * Returns front element without removing it.
     * Returns true if successful, false if buffer is empty.
     */
    [[nodiscard]]
    bool peek(T &out) const {
        if (is_empty()) {
            return false;
        }
        out = buffer_[head_];
        return true;
    }

   private:
    T buffer_[Capacity];
    size_t head_ = 0;   // Read position
    size_t tail_ = 0;   // Write position
    size_t count_ = 0;  // Cached size
};
