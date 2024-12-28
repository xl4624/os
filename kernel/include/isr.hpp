#pragma once

struct interrupt_frame;
__attribute__((interrupt)) void isr0_handler(struct interrupt_frame *frame);
