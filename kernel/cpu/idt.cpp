#include "idt.hpp"

#include <string.h>

namespace IDT {
    __attribute__((aligned(0x10))) static Entry idt[256];
    static Descriptor idtp;

    void init() {
        idtp.size = (sizeof(Entry) * 256) - 1;
        idtp.offset = reinterpret_cast<uintptr_t>(&idt);
        memset(&idt, 0, sizeof(idt));
        asm volatile("lidt %0" : : "m"(idtp));
    }

    void set_entry(size_t index, uintptr_t handler, Gate gate, Ring ring) {
        idt[index] = Entry(handler, gate, ring);
    }
}  // namespace IDT
