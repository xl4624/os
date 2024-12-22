#include "tty.hpp"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__) || !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

extern "C" void kernel_main(void) {
    Terminal terminal;

    terminal.writeString("Hello world!\n");
}
