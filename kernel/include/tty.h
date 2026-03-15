#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

void terminal_putchar(char c);
void terminal_clear(void);
void terminal_set_position(unsigned int row, unsigned int col);
void terminal_set_color(unsigned char color);

__END_DECLS

#ifdef __cplusplus
#include <span.h>

void terminal_write(const char* data);
void terminal_write(std::span<const char> data);
#endif
