#pragma once

#include <cstddef.h>

void* operator new(std::size_t size);
void* operator new[](std::size_t size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, std::size_t) noexcept;
void operator delete[](void* ptr, std::size_t) noexcept;

// Placement new: constructs an object at an existing memory address.
inline void* operator new(std::size_t, void* ptr) noexcept { return ptr; }
inline void* operator new[](std::size_t, void* ptr) noexcept { return ptr; }
