#include "ktest.h"

#include <stdio.h>
#include <string.h>
#include <sys/io.h>

TestCase kTests[kMaxTests];
int kTestCount = 0;
TestState kTestState;

// PS/2 Controller ports
static constexpr uint16_t PS2_STATUS_PORT = 0x64;
static constexpr uint16_t PS2_DATA_PORT = 0x60;

// PS/2 Status register bits
static constexpr uint8_t PS2_STATUS_OUTPUT_FULL = 0x01;

// QEMU isa-debug-exit port (for automated testing)
static constexpr uint16_t QEMU_DEBUG_EXIT_PORT = 0xF4;
// isa-debug-exit computes QEMU exit status as (value << 1) | 1.
// Value 0x00 -> exit 1 (Makefile treats as success).
// Value 0x01 -> exit 3 (Makefile treats as failure).
static constexpr uint8_t QEMU_EXIT_CODE_SUCCESS = 0x00;
static constexpr uint8_t QEMU_EXIT_CODE_FAILURE = 0x01;

// QEMU debugcon port. Writing a byte here appears on the host when QEMU is
// started with "-debugcon file:<path>" or "-debugcon stdio".
static constexpr uint16_t DEBUGCON_PORT = 0xE9;

static void dbg_putchar(char c) {
    outb(DEBUGCON_PORT, static_cast<uint8_t>(c));
}

static void dbg_puts(const char *s) {
    while (*s) {
        dbg_putchar(*s++);
    }
}

// Tiny decimal-only int-to-string for test counts.
static void dbg_putint(int n) {
    if (n < 0) {
        dbg_putchar('-');
        n = -n;
    }
    char buf[12];
    int i = 0;
    do {
        buf[i++] = static_cast<char>('0' + n % 10);
        n /= 10;
    } while (n > 0);
    while (i > 0) {
        dbg_putchar(buf[--i]);
    }
}

void ktest_dbg_assert_fail(const char *expr, const char *file, int line) {
    dbg_puts("    ASSERT FAILED: ");
    dbg_puts(expr);
    dbg_puts(" at ");
    dbg_puts(file);
    dbg_putchar(':');
    dbg_putint(line);
    dbg_putchar('\n');
}

namespace KTest {

    void run_all() {
        dbg_puts("=== Kernel Test Suite ===\n");

        char current_category[64] = "";

        for (int i = 0; i < kTestCount; ++i) {
            TestCase &tc = kTests[i];

            // Parse "category/test_name"
            const char *slash = strchr(tc.name, '/');
            if (!slash) {
                continue;
            }

            size_t cat_len = static_cast<size_t>(slash - tc.name);
            char category[64] = {};
            for (size_t j = 0; j < cat_len && j < 63; ++j)
                category[j] = tc.name[j];
            const char *test_name = slash + 1;

            if (strcmp(category, current_category) != 0) {
                dbg_puts("\n[");
                dbg_puts(category);
                dbg_puts("]\n");
                strcpy(current_category, category);
            }

            dbg_puts("  ");
            dbg_puts(test_name);
            dbg_puts(" ... ");

            kTestState.current_failed = false;
            tc.func();

            if (!kTestState.current_failed) {
                dbg_puts("OK\n");
                ++kTestState.passed;
            } else {
                dbg_puts("FAILED\n");
                ++kTestState.failed;
            }
        }

        dbg_puts("\nTotal: ");
        dbg_putint(kTestState.passed);
        dbg_puts(" passed, ");
        dbg_putint(kTestState.failed);
        dbg_puts(" failed\n");

        uint8_t exit_code = (kTestState.failed == 0) ? QEMU_EXIT_CODE_SUCCESS
                                                     : QEMU_EXIT_CODE_FAILURE;
        outb(QEMU_DEBUG_EXIT_PORT, exit_code);
    }
}  // namespace KTest
