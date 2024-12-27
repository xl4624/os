#include <stdio.h>
#include <string.h>

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__) || !defined(__i386__)
    #error "ix86-elf cross compiler required"
#endif

extern "C" void kernel_main(void) {
    printf("Hello world!\n");
    char *test1 = "testing";
    char *test2 = "testimg";
    char c = 'a';
    int r = memcmp((void *)test1, (void *)test2, 7);
    printf("memcmp test: %d\n", r);
    printf("testing: %s\n", test2);
    printf("testing: %d\n", 32);
    printf("testing: %c test\n", c);
}
