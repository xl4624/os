#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void terminal_writestring(const char *data);
void terminal_write(const char *data, size_t size);
void terminal_putchar(char c);

#ifdef __cplusplus
}
#endif
