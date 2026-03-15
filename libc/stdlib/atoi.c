#include <stdlib.h>

int __libc_atoi(const char* nptr) { return (int)strtol(nptr, (char**)0, 10); }
