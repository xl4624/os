#pragma once

#include <cstddef.h>

void *operator new(std::size_t size);
void *operator new[](std::size_t size);
void operator delete(void *ptr) noexcept;
void operator delete[](void *ptr) noexcept;
void operator delete(void *ptr, std::size_t) noexcept;
void operator delete[](void *ptr, std::size_t) noexcept;
