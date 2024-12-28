#include <stdio.h>

struct interrupt_frame;

// TODO: Actually handle divide by zero error don't just hang infinitely
__attribute__((interrupt)) void isr0_handler(struct interrupt_frame *frame) {
    printf("Divide by zero error!\n");
    __asm__ volatile ("cli; hlt");
}
