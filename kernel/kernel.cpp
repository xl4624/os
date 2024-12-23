#include "tty.hpp"
#include <string.h>
/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__) || !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

extern "C" void kernel_main(void) {
    Terminal terminal;

    terminal.writeString("Hello world!\n");
    char *test1 = "testing";
    char *test2 = "testimg";

    int r = memcmp((void*)test1, (void*)test2,7);
    if (r < 0) {
        terminal.writeString("less than");
    } else if (r > 0) {
        terminal.writeString("greater than");
    } else {
        terminal.writeString("equal");
    }
}   
