#include <new.h>
#include <stdlib.h>

void* operator new(std::size_t size) { return malloc(size); }

void* operator new[](std::size_t size) { return malloc(size); }

void operator delete(void* ptr) noexcept { free(ptr); }

void operator delete[](void* ptr) noexcept { free(ptr); }

void operator delete(void* ptr, std::size_t) noexcept { free(ptr); }

void operator delete[](void* ptr, std::size_t) noexcept { free(ptr); }
