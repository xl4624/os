#pragma once

#include <stdint.h>

#define PAGE_SIZE 4096

class PhysicalMemoryManager {
   public:
    PhysicalMemoryManager();
    ~PhysicalMemoryManager() = default;
    PhysicalMemoryManager(const PhysicalMemoryManager &) = delete;
    PhysicalMemoryManager &operator=(const PhysicalMemoryManager &) = delete;

    void* allocate_page();
    void free_page(void* addr);

   private:
    uint32_t *bitmap_ = nullptr;
    uint32_t used_pages_ = 0;
    uint32_t total_pages_ = 0;
    uint32_t total_memory_ = 0;
};
