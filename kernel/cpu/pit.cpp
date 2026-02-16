#include "pit.h"

#include <assert.h>
#include <sys/io.h>

#include "interrupt.h"

static constexpr uint16_t PIT_CHANNEL0_DATA = 0x40;
static constexpr uint16_t PIT_COMMAND = 0x43;

// Command byte = 0x34 = 0b00_11_010_0
//   bits 7-6: 00      = channel 0
//   bits 5-4: 11      = access lobyte then hibyte
//   bits 3-1: 010     = mode 2 (rate generator — reloads divisor automatically)
//   bit  0:   0       = 16-bit binary (not BCD)
static constexpr uint8_t PIT_CMD_CHANNEL0_RATE = 0x34;

static constexpr uint32_t PIT_BASE_FREQ = 1193182;
static constexpr uint32_t PIT_TARGET_HZ = 100;
static constexpr uint16_t PIT_DIVISOR =
    static_cast<uint16_t>(PIT_BASE_FREQ / PIT_TARGET_HZ);  // 11932

namespace {

    volatile uint64_t ticks = 0;
    bool initialized = false;

    void pit_handler([[maybe_unused]] interrupt_frame *frame) {
        ++ticks;
    }

}  // namespace

namespace PIT {

    void init() {
        assert(Interrupt::is_initialized()
               && "PIT::init(): Interrupt must be initialized first");
        assert(!initialized && "PIT::init(): called more than once");

        outb(PIT_COMMAND, PIT_CMD_CHANNEL0_RATE);
        outb(PIT_CHANNEL0_DATA, static_cast<uint8_t>(PIT_DIVISOR & 0xFF));
        outb(PIT_CHANNEL0_DATA, static_cast<uint8_t>(PIT_DIVISOR >> 8));

        Interrupt::register_handler(IRQ::Timer, pit_handler);
        initialized = true;
    }

    uint64_t get_ticks() {
        assert(initialized && "PIT::get_ticks(): called before PIT::init()");
        return ticks;
    }

    void sleep_ticks(uint64_t n) {
        assert(initialized && "PIT::sleep_ticks(): called before PIT::init()");
        const uint64_t target = ticks + n;
        while (ticks < target) {
            asm volatile("hlt");
        }
    }

    void sleep_ms(uint32_t ms) {
        assert(initialized && "PIT::sleep_ms(): called before PIT::init()");
        sleep_ticks((static_cast<uint64_t>(ms) + 9) / 10);
    }

}  // namespace PIT
