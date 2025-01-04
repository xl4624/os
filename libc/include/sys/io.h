#ifndef _SYS_IO_H
#define _SYS_IO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void outb(uint16_t port, uint8_t val) {
    asm("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

#ifdef __cplusplus
}
#endif

#endif
