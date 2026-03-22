#pragma once

namespace Fat {

// Mount the FAT filesystem (FAT16 or FAT32) on the primary ATA drive and
// register all files and directories as VFS nodes under /fat/. Must be
// called after Ata::init() and Vfs::init().
void init_vfs();

}  // namespace Fat
