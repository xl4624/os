#pragma once

#include <stddef.h>
#include <stdint.h>

static constexpr size_t kAtaSectorSize = 512;

namespace Ata {

// Detect the primary master IDE drive. Must be called once during kernel init.
void init();

// Returns true if a drive was detected during init.
[[nodiscard]] bool is_present();

// Returns the 28-bit LBA sector count from IDENTIFY data, or 0 if not present.
[[nodiscard]] uint32_t sector_count();

// Read one 512-byte sector at the given LBA into buf. Returns true on success.
[[nodiscard]] bool read_sector(uint32_t lba, uint8_t buf[512]);

// Write one 512-byte sector from buf to the given LBA. Returns true on success.
[[nodiscard]] bool write_sector(uint32_t lba, const uint8_t buf[512]);

}  // namespace Ata
