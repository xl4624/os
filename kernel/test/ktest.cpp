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
// Value 0x00 → exit 1 (Makefile treats as success).
// Value 0x01 → exit 3 (Makefile treats as failure).
static constexpr uint8_t QEMU_EXIT_CODE_SUCCESS = 0x00;
static constexpr uint8_t QEMU_EXIT_CODE_FAILURE = 0x01;

namespace KTest {

    void run_all() {
        printf("\n=== Kernel Test Suite ===\n");

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
                printf("\n[%s]\n", category);
                strcpy(current_category, category);
            }

            printf("  %-42s... ", test_name);

            kTestState.current_failed = false;
            tc.func();

            if (!kTestState.current_failed) {
                printf("OK\n");
                ++kTestState.passed;
            } else {
                ++kTestState.failed;
            }
        }

        printf("\n==============================\n");
        printf("Total: %d passed, %d failed\n", kTestState.passed,
               kTestState.failed);
        printf("==============================\n");

        constexpr bool debug = false;
        if (debug) {
            printf("\nPress any key to exit...");
            while ((inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) == 0) {
                // Wait for key
            }
            inb(PS2_DATA_PORT);
        }

        uint8_t exit_code = (kTestState.failed == 0) ? QEMU_EXIT_CODE_SUCCESS
                                                     : QEMU_EXIT_CODE_FAILURE;
        outb(QEMU_DEBUG_EXIT_PORT, exit_code);
    }
}  // namespace KTest
