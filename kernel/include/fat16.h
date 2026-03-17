#pragma once

namespace Fat16 {

// Mount the FAT16 filesystem on the primary ATA drive and register all root
// directory files as VFS nodes under /fat/<filename>. Must be called after
// Ata::init() and Vfs::init().
void init_vfs();

}  // namespace Fat16
