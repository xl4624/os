#include "interrupt.hpp"

#include <stdio.h>

#include "idt.hpp"
#include "pic.hpp"

handler_t isr_handlers[32] = {nullptr};
handler_t irq_handlers[16] = {nullptr};

template <uint8_t N>
struct ISRWrapper {
    static constexpr const char *messages[] = {
        // clang-format off
        "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
        "Overflow", "Out of Bounds", "Invalid Opcode", "Device Not Available",
        "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS",
        "Segment Not Present", "Stack Segment Fault", "General Protection Fault",
        "Page Fault", "Reserved15", "x87 Floating Point", "Alignment Check",
        "Machine Check", "SIMD Floating Point", "Virtualization Exception",
        "Control Protection Exception", "Reserved22", "Reserved23",
        "Reserved24", "Reserved25", "Reserved26", "Reserved27",
        "Hypervision Injection Exception", "VMM Communication Exception",
        "Security Exception", "Reserved31",
        // clang-format on
    };
    static_assert(sizeof(messages) / sizeof(messages[0]) == 32,
                  "ISRWrapper::messages must have exactly 32 entries");

    static __attribute__((interrupt, noreturn)) void handle(
        interrupt_frame *frame) {
        if (isr_handlers[N]) {
            isr_handlers[N](frame);
        } else {
            printf("%s (#%d)\n", messages[N], N);
            halt_and_catch_fire();
        }
        __builtin_unreachable();
    }
};

template <uint8_t N>
struct IRQWrapper {
    static __attribute__((interrupt)) void handle(interrupt_frame *frame) {
        if (irq_handlers[N])
            irq_handlers[N](frame);
        PIC::send_eoi(N);
    }
};

template <template <uint8_t> class Wrapper, size_t... Is>
struct HandlerArray {
    static constexpr void (*handlers[])(interrupt_frame *) = {
        Wrapper<Is>::handle...};
};

using ISRTable = HandlerArray<ISRWrapper, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                              12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                              24, 25, 26, 27, 28, 29, 30, 31>;

using IRQTable = HandlerArray<IRQWrapper, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                              12, 13, 14, 15>;

static_assert(sizeof(ISRTable::handlers) / sizeof(ISRTable::handlers[0]) == 32,
              "ISR table must have 32 entries");
static_assert(sizeof(IRQTable::handlers) / sizeof(IRQTable::handlers[0]) == 16,
              "IRQ table must have 16 entries");

void interrupt_init() {
    IDT::init();
    PIC::init();

    // Register default ISR/IRQ handlers
    for (uint8_t i = 0; i < 32; i++) {
        IDT::set_entry(i, reinterpret_cast<size_t>(ISRTable::handlers[i]),
                       IDT::Gate::Trap, IDT::Ring::Kernel);
    }
    for (uint8_t i = 0; i < 16; i++) {
        IDT::set_entry(i + 32, reinterpret_cast<size_t>(IRQTable::handlers[i]),
                       IDT::Gate::Interrupt, IDT::Ring::Kernel);
    }

    interrupt_enable();
}

void exception_register_handler(ISR isr, handler_t handler) {
    isr_handlers[static_cast<uint8_t>(isr)] = handler;
}

void interrupt_register_handler(IRQ irq, handler_t handler) {
    irq_handlers[static_cast<uint8_t>(irq)] = handler;
    PIC::unmask(static_cast<uint8_t>(irq));
}

__BEGIN_DECLS

void interrupt_enable() {
    asm volatile("sti");
}

void interrupt_disable() {
    asm volatile("cli");
}

__attribute__((noreturn)) void halt_and_catch_fire() {
    interrupt_disable();
    asm volatile("hlt");
    __builtin_unreachable();
}

__END_DECLS
