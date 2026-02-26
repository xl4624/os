#include "ktest.h"
#include "ring_buffer.h"

// ===========================================================================
// State queries on a fresh buffer
// ===========================================================================

TEST(ring_buffer, empty_at_start) {
  RingBuffer<int, 4> rb;
  ASSERT_TRUE(rb.is_empty());
  ASSERT_FALSE(rb.is_full());
  ASSERT_EQ(rb.size(), 0u);
}

TEST(ring_buffer, capacity_matches_template) {
  ASSERT_EQ((RingBuffer<int, 4>::capacity()), 4u);
  ASSERT_EQ((RingBuffer<int, 16>::capacity()), 16u);
  ASSERT_EQ((RingBuffer<char, 1>::capacity()), 1u);
}

// ===========================================================================
// Basic push / pop
// ===========================================================================

TEST(ring_buffer, push_single_succeeds) {
  RingBuffer<int, 4> rb;
  ASSERT_TRUE(rb.push(42));
  ASSERT_FALSE(rb.is_empty());
  ASSERT_EQ(rb.size(), 1u);
}

TEST(ring_buffer, pop_single_returns_value) {
  RingBuffer<int, 4> rb;
  ASSERT_TRUE(rb.push(99));
  int val = 0;
  ASSERT_TRUE(rb.pop(val));
  ASSERT_EQ(val, 99);
  ASSERT_TRUE(rb.is_empty());
}

TEST(ring_buffer, pop_fails_when_empty) {
  RingBuffer<int, 4> rb;
  int val = 0;
  ASSERT_FALSE(rb.pop(val));
}

// ===========================================================================
// FIFO ordering
// ===========================================================================

TEST(ring_buffer, fifo_ordering) {
  RingBuffer<int, 8> rb;
  for (int i = 0; i < 5; ++i) {
    ASSERT_TRUE(rb.push(i * 10));
  }
  for (int i = 0; i < 5; ++i) {
    int val = -1;
    ASSERT_TRUE(rb.pop(val));
    ASSERT_EQ(val, i * 10);
  }
}

// ===========================================================================
// Full detection and push failure
// ===========================================================================

TEST(ring_buffer, full_after_capacity_pushes) {
  RingBuffer<int, 4> rb;
  for (int i = 0; i < 4; ++i) {
    ASSERT_TRUE(rb.push(i));
  }
  ASSERT_TRUE(rb.is_full());
  ASSERT_EQ(rb.size(), 4u);
}

TEST(ring_buffer, push_fails_when_full) {
  RingBuffer<int, 2> rb;
  ASSERT_TRUE(rb.push(1));
  ASSERT_TRUE(rb.push(2));
  ASSERT_FALSE(rb.push(3));
  ASSERT_EQ(rb.size(), 2u);
  // Original elements are untouched.
  int val = 0;
  ASSERT_TRUE(rb.pop(val));
  ASSERT_EQ(val, 1);
  ASSERT_TRUE(rb.pop(val));
  ASSERT_EQ(val, 2);
}

// ===========================================================================
// Peek
// ===========================================================================

TEST(ring_buffer, peek_returns_front_without_removing) {
  RingBuffer<int, 4> rb;
  ASSERT_TRUE(rb.push(7));
  ASSERT_TRUE(rb.push(8));
  int val = 0;
  ASSERT_TRUE(rb.peek(val));
  ASSERT_EQ(val, 7);
  ASSERT_EQ(rb.size(), 2u);
  // Second peek still returns the same front element.
  ASSERT_TRUE(rb.peek(val));
  ASSERT_EQ(val, 7);
}

TEST(ring_buffer, peek_fails_when_empty) {
  RingBuffer<int, 4> rb;
  int val = 0;
  ASSERT_FALSE(rb.peek(val));
}

// ===========================================================================
// Clear
// ===========================================================================

TEST(ring_buffer, clear_resets_state) {
  RingBuffer<int, 4> rb;
  ASSERT_TRUE(rb.push(1));
  ASSERT_TRUE(rb.push(2));
  ASSERT_TRUE(rb.push(3));
  rb.clear();
  ASSERT_TRUE(rb.is_empty());
  ASSERT_FALSE(rb.is_full());
  ASSERT_EQ(rb.size(), 0u);
  // Can push again after clear.
  ASSERT_TRUE(rb.push(10));
  int val = 0;
  ASSERT_TRUE(rb.pop(val));
  ASSERT_EQ(val, 10);
}

// ===========================================================================
// Wrap-around
// ===========================================================================

TEST(ring_buffer, wrap_around) {
  RingBuffer<int, 4> rb;
  // Fill and drain three times to force multiple internal wrap-arounds.
  for (int round = 0; round < 3; ++round) {
    for (int i = 0; i < 4; ++i) {
      ASSERT_TRUE(rb.push(round * 10 + i));
    }
    for (int i = 0; i < 4; ++i) {
      int val = 0;
      ASSERT_TRUE(rb.pop(val));
      ASSERT_EQ(val, round * 10 + i);
    }
  }
  ASSERT_TRUE(rb.is_empty());
}

TEST(ring_buffer, interleaved_push_pop) {
  RingBuffer<int, 4> rb;
  ASSERT_TRUE(rb.push(1));
  ASSERT_TRUE(rb.push(2));
  int val = 0;
  ASSERT_TRUE(rb.pop(val));
  ASSERT_EQ(val, 1);
  // Tail has wrapped: push 3 and 4 to make buffer [2, 3, 4, _].
  ASSERT_TRUE(rb.push(3));
  ASSERT_TRUE(rb.push(4));
  ASSERT_TRUE(rb.push(5));  // now full: [2, 3, 4, 5]
  ASSERT_TRUE(rb.is_full());
  for (int expected = 2; expected <= 5; ++expected) {
    ASSERT_TRUE(rb.pop(val));
    ASSERT_EQ(val, expected);
  }
  ASSERT_TRUE(rb.is_empty());
}

// ===========================================================================
// size() tracks correctly through mixed operations
// ===========================================================================

TEST(ring_buffer, size_tracks_through_operations) {
  RingBuffer<int, 8> rb;
  ASSERT_EQ(rb.size(), 0u);
  ASSERT_TRUE(rb.push(1));
  ASSERT_EQ(rb.size(), 1u);
  ASSERT_TRUE(rb.push(2));
  ASSERT_TRUE(rb.push(3));
  ASSERT_EQ(rb.size(), 3u);
  int val = 0;
  ASSERT_TRUE(rb.pop(val));
  ASSERT_EQ(rb.size(), 2u);
  rb.clear();
  ASSERT_EQ(rb.size(), 0u);
}
