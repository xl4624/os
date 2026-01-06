#ifndef _SYS_IO_H
#define _SYS_IO_H

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

[[nodiscard]] inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

inline void io_wait(void) {
    outb(0x80, 0);
}

__END_DECLS

#endif /* _SYS_IO_H */
