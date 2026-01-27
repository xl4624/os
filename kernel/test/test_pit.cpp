#include "ktest.hpp"
#include "pit.hpp"

TEST(pit, ticks_advance) {
    uint64_t t0 = PIT::get_ticks();
    // Wait for at least one timer interrupt via hlt.
    asm volatile("hlt");
    uint64_t t1 = PIT::get_ticks();
    ASSERT(t1 > t0);
}

TEST(pit, sleep_ms_waits) {
    uint64_t t0 = PIT::get_ticks();
    PIT::sleep_ms(100);  // ~10 ticks at 100 Hz
    uint64_t t1 = PIT::get_ticks();
    uint64_t delta = t1 - t0;
    // Should be approximately 10 ticks (100ms / 10ms per tick), ±5.
    ASSERT(abs(static_cast<int>(delta) - 10) <= 5);
}
