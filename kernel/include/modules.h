#pragma once

#include <stddef.h>
#include <stdint.h>

#include "multiboot.h"

// Maximum number of multiboot modules supported.
static constexpr uint32_t kMaxModules = 16;

// A single in-memory binary registered from a multiboot module.
struct Module {
    const char *name;     // basename of the GRUB cmdline (e.g. "tetris.elf")
    const uint8_t *data;  // virtual address of module data
    size_t len;           // length in bytes
};

namespace Modules {

    // Parse the multiboot module list and populate the registry.
    // Must be called after paging is active (uses phys_to_virt).
    void init(const multiboot_info_t *info);

    // Return a module by name, or nullptr if not found.
    const Module *find(const char *name);

    // Number of modules registered.
    uint32_t count();

    // Access a module by index (0-based). Returns nullptr if out of range.
    const Module *get(uint32_t index);

}  // namespace Modules
