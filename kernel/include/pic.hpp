#pragma once

#include <stdint.h>

void pic_init();
void pic_sendEOI(uint8_t irq);
void pic_mask(uint8_t irq);
void pic_unmask(uint8_t irq);
void pic_disable();
