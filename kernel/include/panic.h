#pragma once

#include <sys/cdefs.h>

__BEGIN_DECLS

__attribute__((noreturn)) void panic(const char* format, ...);

__END_DECLS
