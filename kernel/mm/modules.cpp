#include "modules.h"

#include <string.h>

#include "paging.h"

namespace {

    uint32_t module_count = 0;
    Module module_table[kMaxModules];

    // Returns a pointer to the last path component (basename).
    // If there is no '/', returns the full string.
    const char *basename(const char *path) {
        const char *last = path;
        for (const char *p = path; *p; ++p) {
            if (*p == '/') {
                last = p + 1;
            }
        }
        return last;
    }

}  // namespace

namespace Modules {

    void init(const multiboot_info_t *info) {
        if (!(info->flags & MULTIBOOT_INFO_MODS) || info->mods_count == 0) {
            return;
        }

        const auto *mods =
            phys_to_virt(paddr_t{info->mods_addr}).ptr<multiboot_module_t>();

        uint32_t n = info->mods_count;
        if (n > kMaxModules) {
            n = kMaxModules;
        }

        for (uint32_t i = 0; i < n; ++i) {
            const char *name = "";
            if (mods[i].cmdline != 0) {
                const char *cmdline =
                    phys_to_virt(paddr_t{mods[i].cmdline}).ptr<const char>();
                name = basename(cmdline);
            }

            module_table[i].name = name;
            module_table[i].data =
                phys_to_virt(paddr_t{mods[i].mod_start}).ptr<const uint8_t>();
            module_table[i].len = mods[i].mod_end - mods[i].mod_start;
        }

        module_count = n;
    }

    const Module *find(const char *name) {
        for (uint32_t i = 0; i < module_count; ++i) {
            if (strcmp(module_table[i].name, name) == 0) {
                return &module_table[i];
            }
        }
        return nullptr;
    }

    uint32_t count() {
        return module_count;
    }

    const Module *get(uint32_t index) {
        if (index >= module_count) {
            return nullptr;
        }
        return &module_table[index];
    }

}  // namespace Modules
