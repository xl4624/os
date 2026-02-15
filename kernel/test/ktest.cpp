#include "ktest.hpp"

#include <stdio.h>
#include <string.h>
#include <sys/io.h>

#include "keyboard.hpp"

KTestCase g_ktests[KTEST_MAX];
int g_ktest_count = 0;
KTestState g_ktest_state;

namespace {
    // PS/2 Controller ports
    constexpr uint16_t PS2_STATUS_PORT = 0x64;
    constexpr uint16_t PS2_DATA_PORT = 0x60;

    // PS/2 Status register bits
    constexpr uint8_t PS2_STATUS_OUTPUT_FULL = 0x01;

    // QEMU isa-debug-exit port (for automated testing)
    constexpr uint16_t QEMU_DEBUG_EXIT_PORT = 0xF4;
    constexpr uint8_t QEMU_EXIT_CODE_SUCCESS = 0x00;
}  // namespace

namespace KTest {

    void run_all() {
        printf("\n=== Kernel Test Suite ===\n");

        char current_category[64] = "";

        for (int i = 0; i < g_ktest_count; ++i) {
            KTestCase &tc = g_ktests[i];

            // Parse "category/test_name"
            const char *slash = strchr(tc.name, '/');
            if (!slash)
                continue;

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

            g_ktest_state.current_failed = false;
            tc.func();

            if (!g_ktest_state.current_failed) {
                printf("OK\n");
                ++g_ktest_state.passed;
            } else {
                ++g_ktest_state.failed;
            }
        }

        printf("\n==============================\n");
        printf("Total: %d passed, %d failed\n", g_ktest_state.passed,
               g_ktest_state.failed);
        printf("==============================\n");

        printf("\nPress any key to exit...\n");

        // Wait for a printable key press (not release, not Enter)
        while (true) {
            // Wait for scancode available
            while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL)) {}

            uint8_t scancode = inb(PS2_DATA_PORT);

            // Check if it's a press (bit 7 not set)
            if ((scancode & 0x80) == 0) {
                Key key = KeyboardDriver::scancode_to_key(scancode);

                // Skip Enter - require a different key
                if (key != Key::Enter) {
                    break;
                }
            }
            // Ignore releases, keep waiting
        }

        outb(QEMU_DEBUG_EXIT_PORT, QEMU_EXIT_CODE_SUCCESS);

        while (1) {
            asm volatile("hlt");
        }
    }

}  // namespace KTest
