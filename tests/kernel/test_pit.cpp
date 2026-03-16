#include "ktest.h"
#include "pit.h"

// ===========================================================================
// PIT::get_ticks
// ===========================================================================

TEST(pit, ticks_advance) {
  const uint64_t t0 = PIT::get_ticks();
  // Wait for at least one timer interrupt via hlt.
  __asm__ volatile("hlt");
  const uint64_t t1 = PIT::get_ticks();
  ASSERT(t1 > t0);
}

// ===========================================================================
// PIT::sleep_ms
// ===========================================================================

TEST(pit, sleep_ms_waits) {
  const uint64_t t0 = PIT::get_ticks();
  PIT::sleep_ms(100);  // ~10 ticks at 100 Hz
  const uint64_t t1 = PIT::get_ticks();
  const uint64_t delta = t1 - t0;
  // Should be approximately 10 ticks (100ms / 10ms per tick), ±5.
  ASSERT(abs(static_cast<int>(delta) - 10) <= 5);
}
