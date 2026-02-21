#include "syscall.h"

int main(void) {
    const char *msg = "Hello from userspace!\n";

    /* Count string length */
    int len = 0;
    while (msg[len])
        len++;

    write(1, msg, len);
    return 0;
}
