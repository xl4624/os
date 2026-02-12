#include <stdlib.h>

#include "../framework/test.h"

// rand() produces values in [0, RAND_MAX].

TEST(rand, returns_nonnegative) {
    srand(1);
    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(rand() >= 0);
    }
}

TEST(rand, returns_at_most_rand_max) {
    srand(42);
    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(rand() <= RAND_MAX);
    }
}

TEST(rand, same_seed_same_sequence) {
    srand(12345);
    int a0 = rand();
    int a1 = rand();
    int a2 = rand();

    srand(12345);
    ASSERT_EQ(rand(), a0);
    ASSERT_EQ(rand(), a1);
    ASSERT_EQ(rand(), a2);
}

TEST(rand, different_seeds_different_first_value) {
    srand(1);
    int v1 = rand();
    srand(2);
    int v2 = rand();
    // Different seeds must produce different first values.
    ASSERT_NE(v1, v2);
}

TEST(rand, sequence_not_constant) {
    srand(1);
    int first = rand();
    // A sequence of 10 calls must contain at least one value != first.
    bool found_different = false;
    for (int i = 0; i < 10; ++i) {
        if (rand() != first) {
            found_different = true;
            break;
        }
    }
    ASSERT_TRUE(found_different);
}

// Seed 0 must not produce a stuck sequence (all-zero or all-same output).
TEST(rand, seed_zero_produces_varied_sequence) {
    srand(0);
    int first = rand();
    bool found_different = false;
    for (int i = 0; i < 32; ++i) {
        if (rand() != first) {
            found_different = true;
            break;
        }
    }
    ASSERT_TRUE(found_different);
}

// 1000 consecutive calls must all stay within [0, RAND_MAX].
TEST(rand, large_sample_stays_in_bounds) {
    srand(42);
    for (int i = 0; i < 1000; ++i) {
        int v = rand();
        ASSERT_TRUE(v >= 0);
        ASSERT_TRUE(v <= RAND_MAX);
    }
}

// Reseeding with the same value must reproduce an identical sequence.
TEST(rand, reseed_reproduces_sequence) {
    int vals[5];
    srand(99999);
    for (int i = 0; i < 5; ++i) {
        vals[i] = rand();
    }
    srand(99999);
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(rand(), vals[i]);
    }
}
