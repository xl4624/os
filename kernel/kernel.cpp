#include <stdio.h>
#include <string.h>

#include "tty.hpp"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__) || !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

extern "C" void kernel_main(void) {
    Terminal &terminal = Terminal::getInstance();

    terminal.writeString("Hello world!\n");
    char *test1 = "testing";
    char *test2 = "testimg";
    char c = 'a';

    int r = memcmp((void *)test1, (void *)test2, 7);
    if (r < 0) {
        terminal.writeString("less than\n");
    } else if (r > 0) {
        terminal.writeString("greater than\n");
    } else {
        terminal.writeString("equal\n");
    }
    printf("testing: %s\n", test2);
    printf("testing: %c\n", c);
}
