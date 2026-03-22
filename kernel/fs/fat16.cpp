#include "fat16.h"

#include <algorithm.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ata.h"
#include "heap.h"
#include "scheduler.h"
#include "vfs.h"

// ===========================================================================
// On-disk structures
// ===========================================================================

// Common BPB fields (offsets 0-35), shared between FAT16 and FAT32.
struct FatBpbCommon {
  uint8_t jump_boot[3];
  char oem_name[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sector_count;
  uint8_t num_fats;
  uint16_t root_entry_count;
  uint16_t total_sectors_16;
  uint8_t media_type;
  uint16_t fat_size_16;  // 0 on FAT32
  uint16_t sectors_per_track;
  uint16_t num_heads;
  uint32_t hidden_sectors;
  uint32_t total_sectors_32;
} __attribute__((packed));
static_assert(sizeof(FatBpbCommon) == 36, "FatBpbCommon size mismatch");

// FAT32 extended BPB at offset 36.
struct Fat32Ebpb {
  uint32_t fat_size_32;
  uint16_t ext_flags;
  uint16_t fs_version;
  uint32_t root_cluster;
  uint16_t fs_info;
  uint16_t backup_boot_sector;
  uint8_t reserved[12];
  uint8_t drive_number;
  uint8_t reserved1;
  uint8_t boot_sig;
  uint32_t volume_id;
  char volume_label[11];
  char fs_type[8];  // "FAT32   "
} __attribute__((packed));
static_assert(sizeof(Fat32Ebpb) == 54, "Fat32Ebpb size mismatch");

// 32-byte FAT directory entry (works for both FAT16 and FAT32).
struct FatDirEntry {
  char name[8];  // 8.3 base name, space-padded
  char ext[3];   // 8.3 extension, space-padded
  uint8_t attr;
  uint8_t nt_res;
  uint8_t crt_time_tenth;
  uint16_t crt_time;
  uint16_t crt_date;
  uint16_t lst_acc_date;
  uint16_t fst_clus_hi;  // high 16 bits of first cluster (FAT32; 0 on FAT16)
  uint16_t write_time;
  uint16_t write_date;
  uint16_t fst_clus_lo;  // low 16 bits of first cluster
  uint32_t file_size;
} __attribute__((packed));
static_assert(sizeof(FatDirEntry) == 32, "FatDirEntry size mismatch");

// Directory entry attribute flags.
static constexpr uint8_t kAttrHidden = 0x02;
static constexpr uint8_t kAttrSystem = 0x04;
static constexpr uint8_t kAttrVolumeId = 0x08;
static constexpr uint8_t kAttrDirectory = 0x10;
static constexpr uint8_t kAttrArchive = 0x20;
static constexpr uint8_t kAttrLfnMask = 0x0F;  // all four low bits set = LFN entry

// FAT end-of-chain sentinels.
static constexpr uint32_t kEoc16 = 0xFFF8U;
static constexpr uint32_t kEoc32 = 0x0FFFFFF8U;

// Per-file metadata stored in VfsNode::priv for FAT file nodes.
struct FatFileInfo {
  uint32_t start_cluster;
  uint32_t file_size;
  uint32_t dir_entry_sector;  // LBA of the sector holding this file's dir entry
  uint32_t dir_entry_offset;  // byte offset of the dir entry within that sector
};

// Per-directory metadata stored in VfsNode::priv for FAT directory nodes.
struct FatDirInfo {
  uint32_t cluster;  // first cluster of dir; 0 = FAT16 fixed root directory
};

namespace {

struct FatState {
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint32_t fat_start;         // first sector of FAT1
  uint32_t root_dir_start;    // first sector of FAT16 root dir region
  uint32_t root_dir_sectors;  // number of sectors in FAT16 root dir
  uint32_t data_start;        // first sector of data region (cluster 2)
  uint16_t root_entry_count;  // FAT16 root dir entry count (0 on FAT32)
  uint32_t root_cluster;      // FAT32: first cluster of root dir
  uint32_t fat_size;          // size of FAT1 in sectors
  uint32_t total_clusters;    // total number of data clusters
  uint8_t* fat;               // heap-allocated copy of FAT1
  bool is_fat32;
  bool mounted;
};

FatState s;

// ===========================================================================
// FAT chain helpers
// ===========================================================================

uint32_t fat_get(uint32_t cluster) {
  assert(s.fat != nullptr && "fat_get(): FAT not loaded");
  if (s.is_fat32) {
    const uint32_t byte_off = cluster * 4U;
    uint32_t val;
    memcpy(&val, s.fat + byte_off, 4);
    return val & 0x0FFFFFFFU;
  }
  const uint32_t byte_off = cluster * 2U;
  uint16_t val;
  memcpy(&val, s.fat + byte_off, 2);
  return val;
}

void fat_set(uint32_t cluster, uint32_t value) {
  assert(s.fat != nullptr && "fat_set(): FAT not loaded");
  if (s.is_fat32) {
    const uint32_t byte_off = cluster * 4U;
    uint32_t old_val;
    memcpy(&old_val, s.fat + byte_off, 4);
    const uint32_t new_val = (old_val & 0xF0000000U) | (value & 0x0FFFFFFFU);
    memcpy(s.fat + byte_off, &new_val, 4);
    const uint32_t fat_sec = byte_off / s.bytes_per_sector;
    (void)Ata::write_sector(s.fat_start + fat_sec, s.fat + (fat_sec * s.bytes_per_sector));
  } else {
    const uint32_t byte_off = cluster * 2U;
    const auto val16 = static_cast<uint16_t>(value);
    memcpy(s.fat + byte_off, &val16, 2);
    const uint32_t fat_sec = byte_off / s.bytes_per_sector;
    (void)Ata::write_sector(s.fat_start + fat_sec, s.fat + (fat_sec * s.bytes_per_sector));
  }
}

bool fat_is_eoc(uint32_t entry) {
  if (s.is_fat32) {
    return (entry & 0x0FFFFFFFU) >= kEoc32;
  }
  return entry >= kEoc16;
}

// Allocate a free cluster (value == 0). Returns 0 if the FAT is full.
uint32_t fat_alloc_cluster() {
  for (uint32_t c = 2; c < s.total_clusters + 2; ++c) {
    if (fat_get(c) == 0) {
      fat_set(c, s.is_fat32 ? 0x0FFFFFFFU : 0xFFFFU);  // mark as EOC
      return c;
    }
  }
  return 0;  // full
}

// Free an entire cluster chain starting at 'start'.
void fat_free_chain(uint32_t start) {
  uint32_t c = start;
  while (c >= 2 && !fat_is_eoc(c)) {
    const uint32_t next = fat_get(c);
    fat_set(c, 0);
    c = next;
  }
}

uint32_t cluster_to_sector(uint32_t cluster) {
  assert(cluster >= 2 && "cluster_to_sector(): invalid cluster number");
  return s.data_start + ((cluster - 2U) * s.sectors_per_cluster);
}

// Extend the cluster chain for fi by one cluster. Returns false if full.
bool append_cluster(FatFileInfo* fi) {
  const uint32_t new_clus = fat_alloc_cluster();
  if (new_clus == 0) {
    return false;
  }

  if (fi->start_cluster == 0) {
    fi->start_cluster = new_clus;
    return true;
  }
  // Walk to last cluster.
  uint32_t c = fi->start_cluster;
  while (!fat_is_eoc(fat_get(c))) {
    c = fat_get(c);
  }
  fat_set(c, new_clus);
  return true;
}

// ===========================================================================
// Directory entry helpers
// ===========================================================================

bool read_dir_entry(uint32_t lba, uint32_t byte_offset, FatDirEntry* out) {
  uint8_t sec[512];
  if (!Ata::read_sector(lba, sec)) {
    return false;
  }
  memcpy(out, sec + byte_offset, sizeof(FatDirEntry));
  return true;
}

bool write_dir_entry(uint32_t lba, uint32_t byte_offset, const FatDirEntry* entry) {
  uint8_t sec[512];
  if (!Ata::read_sector(lba, sec)) {
    return false;
  }
  memcpy(sec + byte_offset, entry, sizeof(FatDirEntry));
  return Ata::write_sector(lba, sec);
}

// Write back the file size and start cluster from fi to its dir entry on disk.
void update_dir_entry(FatFileInfo* fi, VfsNode* node) {
  uint8_t sec[512];
  if (!Ata::read_sector(fi->dir_entry_sector, sec)) {
    return;
  }
  auto* de = reinterpret_cast<FatDirEntry*>(sec + fi->dir_entry_offset);
  de->file_size = fi->file_size;
  de->fst_clus_lo = static_cast<uint16_t>(fi->start_cluster & 0xFFFFU);
  de->fst_clus_hi = static_cast<uint16_t>((fi->start_cluster >> 16) & 0xFFFFU);
  (void)Ata::write_sector(fi->dir_entry_sector, sec);
  node->size = fi->file_size;
}

// Extract first cluster from a dir entry (works for FAT16 and FAT32).
uint32_t de_cluster(const FatDirEntry& de) {
  return (static_cast<uint32_t>(de.fst_clus_hi) << 16) | de.fst_clus_lo;
}

// Convert 8.3 name/ext pair to lowercase null-terminated string.
void format_name(const char name8[8], const char ext3[3], char* out, size_t max_len) {
  size_t pos = 0;
  for (int i = 0; i < 8 && name8[i] != ' '; ++i) {
    if (pos + 1 < max_len) {
      out[pos++] = static_cast<char>(tolower(static_cast<unsigned char>(name8[i])));
    }
  }
  bool has_ext = false;
  for (int i = 0; i < 3; ++i) {
    if (ext3[i] != ' ') {
      has_ext = true;
      break;
    }
  }
  if (has_ext) {
    if (pos + 1 < max_len) {
      out[pos++] = '.';
    }
    for (int i = 0; i < 3 && ext3[i] != ' '; ++i) {
      if (pos + 1 < max_len) {
        out[pos++] = static_cast<char>(tolower(static_cast<unsigned char>(ext3[i])));
      }
    }
  }
  out[pos] = '\0';
}

// Convert a filename like "file.txt" to 8.3 name[8] / ext[3] fields.
// Returns false if the name is invalid or too long.
bool str_to_83(const char* fname, char name8[8], char ext3[3]) {
  memset(name8, ' ', 8);
  memset(ext3, ' ', 3);
  if (fname == nullptr || fname[0] == '\0') {
    return false;
  }

  const char* dot = strchr(fname, '.');
  const size_t base_len = dot != nullptr ? static_cast<size_t>(dot - fname) : strlen(fname);
  if (base_len == 0 || base_len > 8) {
    return false;
  }

  for (size_t i = 0; i < base_len; ++i) {
    name8[i] = static_cast<char>(toupper(static_cast<unsigned char>(fname[i])));
  }

  if (dot != nullptr) {
    const char* ext = dot + 1;
    const size_t ext_len = strlen(ext);
    if (ext_len > 3) {
      return false;
    }
    for (size_t i = 0; i < ext_len; ++i) {
      ext3[i] = static_cast<char>(toupper(static_cast<unsigned char>(ext[i])));
    }
  }
  return true;
}

// Scan dir sectors (either fixed root or cluster chain) for a free slot.
// dir_cluster == 0 means FAT16 fixed root directory.
// Returns true and sets *out_lba, *out_offset on success.
bool find_free_dir_slot(uint32_t dir_cluster, uint32_t* out_lba, uint32_t* out_offset) {
  const uint32_t bps = s.bytes_per_sector;
  const uint32_t entries_per_sector = bps / sizeof(FatDirEntry);

  auto scan_sector = [&](uint32_t lba) -> bool {
    uint8_t sec[512];
    if (!Ata::read_sector(lba, sec)) {
      return false;
    }
    for (uint32_t e = 0; e < entries_per_sector; ++e) {
      const auto first = sec[e * sizeof(FatDirEntry)];
      if (first == 0x00 || first == 0xE5) {
        *out_lba = lba;
        *out_offset = e * static_cast<uint32_t>(sizeof(FatDirEntry));
        return true;
      }
    }
    return false;
  };

  if (dir_cluster == 0) {
    // FAT16 fixed root directory.
    for (uint32_t sec = 0; sec < s.root_dir_sectors; ++sec) {
      if (scan_sector(s.root_dir_start + sec)) {
        return true;
      }
    }
    return false;
  }

  // Cluster-chain directory.
  uint32_t cluster = dir_cluster;
  while (cluster >= 2 && !fat_is_eoc(cluster)) {
    const uint32_t base = cluster_to_sector(cluster);
    for (uint32_t sec = 0; sec < s.sectors_per_cluster; ++sec) {
      if (scan_sector(base + sec)) {
        return true;
      }
    }
    cluster = fat_get(cluster);
  }
  return false;
}

// ===========================================================================
// VfsOps callbacks
// ===========================================================================

int32_t fat_read(VfsNode* node, std::span<uint8_t> buf, uint32_t offset) {
  assert(s.mounted && "fat_read(): filesystem not mounted");
  auto* fi = static_cast<FatFileInfo*>(node->priv);
  assert(fi != nullptr && "fat_read(): node missing FatFileInfo");

  if (offset >= fi->file_size || fi->start_cluster == 0) {
    return 0;
  }

  const uint32_t bps = s.bytes_per_sector;
  const uint32_t spc = s.sectors_per_cluster;
  const uint32_t bytes_per_cluster = bps * spc;

  const auto buf_len = static_cast<uint32_t>(buf.size());
  const uint32_t available = fi->file_size - offset;
  uint32_t remaining = std::min(buf_len, available);
  uint32_t out_pos = 0;

  const uint32_t cluster_idx = offset / bytes_per_cluster;
  uint32_t cluster_off = offset % bytes_per_cluster;

  uint32_t cluster = fi->start_cluster;
  for (uint32_t i = 0; i < cluster_idx; ++i) {
    cluster = fat_get(cluster);
    if (cluster < 2 || fat_is_eoc(cluster)) {
      return 0;
    }
  }

  uint8_t sector_buf[512];

  while (remaining > 0 && cluster >= 2 && !fat_is_eoc(cluster)) {
    const uint32_t clus_sector = cluster_to_sector(cluster);
    uint32_t sector_idx = cluster_off / bps;
    uint32_t byte_in_sec = cluster_off % bps;

    while (remaining > 0 && sector_idx < spc) {
      if (!Ata::read_sector(clus_sector + sector_idx, sector_buf)) {
        return -1;
      }
      const uint32_t sec_avail = bps - byte_in_sec;
      const uint32_t to_copy = std::min(remaining, sec_avail);
      memcpy(buf.data() + out_pos, sector_buf + byte_in_sec, to_copy);
      out_pos += to_copy;
      remaining -= to_copy;
      byte_in_sec = 0;
      ++sector_idx;
    }

    cluster_off = 0;
    cluster = fat_get(cluster);
  }

  return static_cast<int32_t>(out_pos);
}

int32_t fat_write(VfsNode* node, std::span<const uint8_t> buf, uint32_t offset) {
  assert(s.mounted && "fat_write(): filesystem not mounted");
  auto* fi = static_cast<FatFileInfo*>(node->priv);
  assert(fi != nullptr && "fat_write(): node missing FatFileInfo");

  if (buf.empty()) {
    return 0;
  }

  const uint32_t bps = s.bytes_per_sector;
  const uint32_t spc = s.sectors_per_cluster;
  const uint32_t bytes_per_cluster = bps * spc;

  const uint32_t end = offset + static_cast<uint32_t>(buf.size());

  // Extend cluster chain if needed.
  uint32_t allocated = 0;
  if (fi->start_cluster != 0) {
    uint32_t c = fi->start_cluster;
    allocated = bytes_per_cluster;
    while (!fat_is_eoc(fat_get(c))) {
      c = fat_get(c);
      allocated += bytes_per_cluster;
    }
  }

  while (allocated < end) {
    if (!append_cluster(fi)) {
      return -ENOSPC;
    }
    allocated += bytes_per_cluster;
  }

  // Walk chain to the cluster containing 'offset'.
  const uint32_t cluster_idx = offset / bytes_per_cluster;
  uint32_t cluster = fi->start_cluster;
  for (uint32_t i = 0; i < cluster_idx; ++i) {
    cluster = fat_get(cluster);
    if (cluster < 2 || fat_is_eoc(cluster)) {
      return -EIO;
    }
  }

  uint32_t written = 0;
  uint32_t cur_off = offset % bytes_per_cluster;

  while (written < static_cast<uint32_t>(buf.size()) && cluster >= 2 && !fat_is_eoc(cluster)) {
    const uint32_t clus_sector = cluster_to_sector(cluster);
    uint32_t sector_idx = cur_off / bps;
    uint32_t byte_in_sec = cur_off % bps;

    while (written < static_cast<uint32_t>(buf.size()) && sector_idx < spc) {
      const uint32_t sec_avail = bps - byte_in_sec;
      const uint32_t to_write = std::min(static_cast<uint32_t>(buf.size()) - written, sec_avail);
      const uint32_t lba = clus_sector + sector_idx;

      uint8_t sec[512];
      if (!Ata::read_sector(lba, sec)) {
        return -EIO;
      }
      memcpy(sec + byte_in_sec, buf.data() + written, to_write);
      if (!Ata::write_sector(lba, sec)) {
        return -EIO;
      }

      written += to_write;
      byte_in_sec = 0;
      ++sector_idx;
    }

    cur_off = 0;
    cluster = fat_get(cluster);
  }

  const uint32_t new_end = offset + written;
  fi->file_size = std::max(new_end, fi->file_size);
  update_dir_entry(fi, node);
  return static_cast<int32_t>(written);
}

int32_t fat_truncate(VfsNode* node) {
  assert(s.mounted && "fat_truncate(): filesystem not mounted");
  auto* fi = static_cast<FatFileInfo*>(node->priv);
  assert(fi != nullptr && "fat_truncate(): node missing FatFileInfo");

  if (fi->start_cluster != 0) {
    fat_free_chain(fi->start_cluster);
    fi->start_cluster = 0;
  }
  fi->file_size = 0;
  update_dir_entry(fi, node);
  return 0;
}

const VfsOps fat_ops = {
    .read = fat_read,
    .write = fat_write,
    .ioctl = nullptr,
    .truncate = fat_truncate,
};

// ===========================================================================
// Recursive directory scanner
// ===========================================================================

// Forward declaration.
void scan_directory(uint32_t dir_sector, uint32_t dir_sectors, uint32_t dir_cluster,
                    const char* vfs_prefix);

void process_dir_entries(const uint8_t* sector_data, uint32_t lba, const char* vfs_prefix,
                         bool* done) {
  static constexpr uint32_t kEntriesPerSector = 512 / sizeof(FatDirEntry);
  const auto* entries = reinterpret_cast<const FatDirEntry*>(sector_data);

  for (uint32_t e = 0; e < kEntriesPerSector; ++e) {
    const FatDirEntry& de = entries[e];

    if (static_cast<uint8_t>(de.name[0]) == 0x00) {
      *done = true;
      return;
    }
    if (static_cast<uint8_t>(de.name[0]) == 0xE5) {
      continue;
    }

    // Skip volume label, LFN entries, hidden/system files.
    if ((de.attr & kAttrVolumeId) != 0) {
      continue;
    }
    if ((de.attr & kAttrLfnMask) == kAttrLfnMask) {
      continue;
    }
    if ((de.attr & (kAttrHidden | kAttrSystem)) != 0) {
      continue;
    }

    // Skip dot entries.
    if (de.name[0] == '.' && (de.name[1] == ' ' || de.name[1] == '.')) {
      continue;
    }

    char fname[13];
    format_name(de.name, de.ext, fname, sizeof(fname));
    if (fname[0] == '\0') {
      continue;
    }

    char path[kMaxPathLen];
    const size_t prefix_len = strlen(vfs_prefix);
    strncpy(path, vfs_prefix, kMaxPathLen - 1);
    if (prefix_len + 1 + strlen(fname) >= kMaxPathLen) {
      continue;
    }
    path[prefix_len] = '/';
    strncpy(path + prefix_len + 1, fname, kMaxPathLen - prefix_len - 2);
    path[kMaxPathLen - 1] = '\0';

    const uint32_t dir_entry_sector = lba;
    const uint32_t dir_entry_offset = e * static_cast<uint32_t>(sizeof(FatDirEntry));

    if ((de.attr & kAttrDirectory) != 0) {
      VfsNode* dir_node = Vfs::register_node(path, VfsNodeType::Directory, nullptr);
      if (dir_node != nullptr) {
        auto* di = new FatDirInfo{.cluster = de_cluster(de)};
        dir_node->priv = di;
      }
      // Recurse into subdirectory.
      scan_directory(0, 0, de_cluster(de), path);
    } else {
      VfsNode* node = Vfs::register_node(path, VfsNodeType::File, &fat_ops);
      if (node != nullptr) {
        auto* fi = new FatFileInfo{
            .start_cluster = de_cluster(de),
            .file_size = de.file_size,
            .dir_entry_sector = dir_entry_sector,
            .dir_entry_offset = dir_entry_offset,
        };
        node->priv = fi;
        node->size = de.file_size;
      }
    }
  }
}

void scan_directory(uint32_t dir_sector, uint32_t dir_sectors, uint32_t dir_cluster,
                    const char* vfs_prefix) {
  uint8_t sec[512];
  bool done = false;

  if (dir_cluster == 0) {
    // FAT16 fixed root directory.
    for (uint32_t i = 0; i < dir_sectors && !done; ++i) {
      if (!Ata::read_sector(dir_sector + i, sec)) {
        break;
      }
      process_dir_entries(sec, dir_sector + i, vfs_prefix, &done);
    }
  } else {
    // Cluster-chain directory (FAT32 root or any subdirectory).
    uint32_t cluster = dir_cluster;
    while (cluster >= 2 && !fat_is_eoc(cluster) && !done) {
      const uint32_t base = cluster_to_sector(cluster);
      for (uint32_t i = 0; i < s.sectors_per_cluster && !done; ++i) {
        if (!Ata::read_sector(base + i, sec)) {
          break;
        }
        process_dir_entries(sec, base + i, vfs_prefix, &done);
      }
      cluster = fat_get(cluster);
    }
  }
}

// ===========================================================================
// FsOps callbacks
// ===========================================================================

int32_t fat_mkdir(const char* abs_path) {
  assert(s.mounted && "fat_mkdir(): filesystem not mounted");

  // Derive parent path and filename.
  const char* last_slash = strrchr(abs_path, '/');
  if (last_slash == nullptr || last_slash == abs_path) {
    return -EINVAL;
  }

  char parent[kMaxPathLen];
  const auto parent_len = static_cast<size_t>(last_slash - abs_path);
  if (parent_len >= kMaxPathLen) {
    return -ENAMETOOLONG;
  }
  strncpy(parent, abs_path, parent_len);
  parent[parent_len] = '\0';

  const char* fname = last_slash + 1;
  char name8[8];
  char ext3[3];
  if (!str_to_83(fname, name8, ext3)) {
    return -EINVAL;
  }

  // Get parent dir cluster.
  uint32_t parent_cluster = 0;
  const VfsNode* parent_node = Vfs::lookup(parent);
  if (parent_node != nullptr && parent_node->type == VfsNodeType::Directory) {
    auto* di = static_cast<FatDirInfo*>(parent_node->priv);
    if (di != nullptr) {
      parent_cluster = di->cluster;
    }
  } else if (parent_node == nullptr && strcmp(parent, "/fat") != 0) {
    return -ENOENT;
  }
  if (parent_node != nullptr && parent_node->type != VfsNodeType::Directory) {
    return -ENOTDIR;
  }

  // Allocate a cluster for the new directory.
  const uint32_t new_cluster = fat_alloc_cluster();
  if (new_cluster == 0) {
    return -ENOSPC;
  }

  // Zero-fill the new cluster.
  uint8_t zero[512];
  memset(zero, 0, sizeof(zero));
  const uint32_t base = cluster_to_sector(new_cluster);
  for (uint32_t i = 0; i < s.sectors_per_cluster; ++i) {
    (void)Ata::write_sector(base + i, zero);
  }

  // Write '.' and '..' entries in the new directory cluster.
  uint8_t sec[512];
  memset(sec, 0, sizeof(sec));
  auto* dot = reinterpret_cast<FatDirEntry*>(sec);
  memset(dot->name, ' ', 8);
  memset(dot->ext, ' ', 3);
  dot->name[0] = '.';
  dot->attr = kAttrDirectory;
  dot->fst_clus_lo = static_cast<uint16_t>(new_cluster & 0xFFFFU);
  dot->fst_clus_hi = static_cast<uint16_t>((new_cluster >> 16) & 0xFFFFU);

  auto* dotdot = reinterpret_cast<FatDirEntry*>(sec + sizeof(FatDirEntry));
  memset(dotdot->name, ' ', 8);
  memset(dotdot->ext, ' ', 3);
  dotdot->name[0] = '.';
  dotdot->name[1] = '.';
  dotdot->attr = kAttrDirectory;
  dotdot->fst_clus_lo = static_cast<uint16_t>(parent_cluster & 0xFFFFU);
  dotdot->fst_clus_hi = static_cast<uint16_t>((parent_cluster >> 16) & 0xFFFFU);

  (void)Ata::write_sector(base, sec);

  // Write directory entry in parent.
  uint32_t slot_lba = 0;
  uint32_t slot_off = 0;
  if (!find_free_dir_slot(parent_cluster, &slot_lba, &slot_off)) {
    fat_set(new_cluster, 0);  // free the cluster we just allocated
    return -ENOSPC;
  }

  FatDirEntry de{};
  memcpy(de.name, name8, 8);
  memcpy(de.ext, ext3, 3);
  de.attr = kAttrDirectory;
  de.fst_clus_lo = static_cast<uint16_t>(new_cluster & 0xFFFFU);
  de.fst_clus_hi = static_cast<uint16_t>((new_cluster >> 16) & 0xFFFFU);
  de.file_size = 0;
  if (!write_dir_entry(slot_lba, slot_off, &de)) {
    return -EIO;
  }

  // Register the new directory VFS node.
  VfsNode* node = Vfs::register_node(abs_path, VfsNodeType::Directory, nullptr);
  if (node != nullptr) {
    node->priv = new FatDirInfo{.cluster = new_cluster};
  }
  return 0;
}

int32_t fat_unlink(const char* abs_path) {
  assert(s.mounted && "fat_unlink(): filesystem not mounted");

  VfsNode* node = Vfs::lookup(abs_path);
  if (node == nullptr) {
    return -ENOENT;
  }
  if (node->type != VfsNodeType::File) {
    return -EISDIR;
  }

  // Refuse if any process has this file open.
  if (Scheduler::is_vfs_node_open(node)) {
    return -EBUSY;
  }

  auto* fi = static_cast<FatFileInfo*>(node->priv);
  assert(fi != nullptr && "fat_unlink(): node missing FatFileInfo");

  // Free the cluster chain.
  if (fi->start_cluster != 0) {
    fat_free_chain(fi->start_cluster);
  }

  // Mark dir entry as deleted.
  uint8_t sec[512];
  if (Ata::read_sector(fi->dir_entry_sector, sec)) {
    sec[fi->dir_entry_offset] = static_cast<uint8_t>(0xE5);
    (void)Ata::write_sector(fi->dir_entry_sector, sec);
  }

  delete fi;
  node->priv = nullptr;
  Vfs::unregister_node(abs_path);
  return 0;
}

int32_t fat_rename(const char* old_path, const char* new_path) {
  assert(s.mounted && "fat_rename(): filesystem not mounted");

  VfsNode* node = Vfs::lookup(old_path);
  if (node == nullptr) {
    return -ENOENT;
  }
  if (node->type != VfsNodeType::File) {
    return -EISDIR;
  }

  // Derive new filename.
  const char* last_slash = strrchr(new_path, '/');
  if (last_slash == nullptr) {
    return -EINVAL;
  }
  const char* new_fname = last_slash + 1;

  char name8[8];
  char ext3[3];
  if (!str_to_83(new_fname, name8, ext3)) {
    return -EINVAL;
  }

  // Get new parent dir cluster.
  char new_parent[kMaxPathLen];
  const auto new_parent_len = static_cast<size_t>(last_slash - new_path);
  if (new_parent_len >= kMaxPathLen) {
    return -ENAMETOOLONG;
  }
  strncpy(new_parent, new_path, new_parent_len);
  new_parent[new_parent_len] = '\0';

  uint32_t new_dir_cluster = 0;
  const VfsNode* np = Vfs::lookup(new_parent);
  if (np != nullptr && np->type == VfsNodeType::Directory) {
    auto* di = static_cast<FatDirInfo*>(np->priv);
    if (di != nullptr) {
      new_dir_cluster = di->cluster;
    }
  }

  auto* fi = static_cast<FatFileInfo*>(node->priv);
  assert(fi != nullptr && "fat_rename(): node missing FatFileInfo");

  // Find a free slot in the new parent directory.
  uint32_t new_lba = 0;
  uint32_t new_off = 0;
  if (!find_free_dir_slot(new_dir_cluster, &new_lba, &new_off)) {
    return -ENOSPC;
  }

  // Read old dir entry to copy timestamps.
  FatDirEntry old_de{};
  if (!read_dir_entry(fi->dir_entry_sector, fi->dir_entry_offset, &old_de)) {
    return -EIO;
  }

  // Write new dir entry.
  FatDirEntry new_de = old_de;
  memcpy(new_de.name, name8, 8);
  memcpy(new_de.ext, ext3, 3);
  if (!write_dir_entry(new_lba, new_off, &new_de)) {
    return -EIO;
  }

  // Mark old dir entry as deleted.
  uint8_t sec[512];
  if (Ata::read_sector(fi->dir_entry_sector, sec)) {
    sec[fi->dir_entry_offset] = static_cast<uint8_t>(0xE5);
    (void)Ata::write_sector(fi->dir_entry_sector, sec);
  }

  // Update in-memory state.
  fi->dir_entry_sector = new_lba;
  fi->dir_entry_offset = new_off;
  strncpy(node->name, new_path, kMaxPathLen - 1);
  node->name[kMaxPathLen - 1] = '\0';

  return 0;
}

VfsNode* fat_create(const char* abs_path, [[maybe_unused]] int32_t flags,
                    [[maybe_unused]] mode_t mode) {
  assert(s.mounted && "fat_create(): filesystem not mounted");

  const char* last_slash = strrchr(abs_path, '/');
  if (last_slash == nullptr) {
    return nullptr;
  }

  char parent[kMaxPathLen];
  const auto parent_len = static_cast<size_t>(last_slash - abs_path);
  if (parent_len >= kMaxPathLen) {
    return nullptr;
  }
  strncpy(parent, abs_path, parent_len);
  parent[parent_len] = '\0';

  const char* fname = last_slash + 1;
  char name8[8];
  char ext3[3];
  if (!str_to_83(fname, name8, ext3)) {
    return nullptr;
  }

  // Get parent dir cluster.
  uint32_t dir_cluster = 0;
  const VfsNode* parent_node = Vfs::lookup(parent);
  if (parent_node != nullptr && parent_node->type == VfsNodeType::Directory) {
    auto* di = static_cast<FatDirInfo*>(parent_node->priv);
    if (di != nullptr) {
      dir_cluster = di->cluster;
    }
  } else if (parent_node == nullptr && strcmp(parent, "/fat") != 0) {
    return nullptr;
  }
  if (parent_node != nullptr && parent_node->type != VfsNodeType::Directory) {
    return nullptr;
  }

  // Find free dir slot.
  uint32_t slot_lba = 0;
  uint32_t slot_off = 0;
  if (!find_free_dir_slot(dir_cluster, &slot_lba, &slot_off)) {
    return nullptr;
  }

  // Write new dir entry (empty file, no cluster).
  FatDirEntry de{};
  memcpy(de.name, name8, 8);
  memcpy(de.ext, ext3, 3);
  de.attr = kAttrArchive;
  de.file_size = 0;
  de.fst_clus_lo = 0;
  de.fst_clus_hi = 0;
  if (!write_dir_entry(slot_lba, slot_off, &de)) {
    return nullptr;
  }

  VfsNode* node = Vfs::register_node(abs_path, VfsNodeType::File, &fat_ops);
  if (node == nullptr) {
    return nullptr;
  }

  auto* fi = new FatFileInfo{
      .start_cluster = 0,
      .file_size = 0,
      .dir_entry_sector = slot_lba,
      .dir_entry_offset = slot_off,
  };
  node->priv = fi;
  node->size = 0;
  return node;
}

const FsOps fat_fs_ops = {
    .mkdir = fat_mkdir,
    .unlink = fat_unlink,
    .rename = fat_rename,
    .create = fat_create,
};

}  // namespace

namespace Fat {

void init_vfs() {
  if (!Ata::is_present()) {
    return;
  }

  uint8_t boot_sector[512];
  if (!Ata::read_sector(0, boot_sector)) {
    printf("fat: failed to read boot sector\n");
    return;
  }

  const auto* bpb = reinterpret_cast<const FatBpbCommon*>(boot_sector);

  if (bpb->bytes_per_sector != 512) {
    printf("fat: unsupported sector size %u\n", bpb->bytes_per_sector);
    return;
  }
  if (bpb->sectors_per_cluster == 0) {
    printf("fat: sectors_per_cluster == 0 (corrupt BPB)\n");
    return;
  }

  s.bytes_per_sector = bpb->bytes_per_sector;
  s.sectors_per_cluster = bpb->sectors_per_cluster;
  s.root_entry_count = bpb->root_entry_count;
  s.fat_start = bpb->reserved_sector_count;
  s.is_fat32 = (bpb->fat_size_16 == 0);

  if (s.is_fat32) {
    const auto* ebpb = reinterpret_cast<const Fat32Ebpb*>(boot_sector + 36);
    s.fat_size = ebpb->fat_size_32;
    s.root_cluster = ebpb->root_cluster;
    s.root_dir_start = 0;
    s.root_dir_sectors = 0;
    s.data_start = s.fat_start + (static_cast<uint32_t>(bpb->num_fats) * s.fat_size);
    // Total clusters = (total_sectors - data_start) / spc
    const uint32_t total_sectors = bpb->total_sectors_32 != 0
                                       ? bpb->total_sectors_32
                                       : static_cast<uint32_t>(bpb->total_sectors_16);
    s.total_clusters = (total_sectors - s.data_start) / s.sectors_per_cluster;
  } else {
    s.fat_size = bpb->fat_size_16;
    s.root_cluster = 0;
    s.root_dir_start = s.fat_start + (static_cast<uint32_t>(bpb->num_fats) * s.fat_size);
    const uint32_t root_bytes = static_cast<uint32_t>(bpb->root_entry_count) * sizeof(FatDirEntry);
    s.root_dir_sectors = (root_bytes + 511U) / 512U;
    s.data_start = s.root_dir_start + s.root_dir_sectors;
    const uint32_t total_sectors = bpb->total_sectors_16 != 0
                                       ? static_cast<uint32_t>(bpb->total_sectors_16)
                                       : bpb->total_sectors_32;
    s.total_clusters = (total_sectors - s.data_start) / s.sectors_per_cluster;
  }

  // Cache FAT1 to avoid repeated sector reads on every file access.
  const uint32_t fat_bytes = s.fat_size * 512U;
  s.fat = new uint8_t[fat_bytes];
  if (s.fat == nullptr) {
    printf("fat: out of memory caching FAT\n");
    return;
  }

  for (uint32_t i = 0; i < s.fat_size; ++i) {
    if (!Ata::read_sector(s.fat_start + i, s.fat + (i * 512U))) {
      printf("fat: failed to read FAT sector %u\n", i);
      delete[] s.fat;
      s.fat = nullptr;
      return;
    }
  }

  s.mounted = true;

  // Register the FAT root as a directory node.
  VfsNode* fat_root = Vfs::register_node("/fat", VfsNodeType::Directory, nullptr);
  if (fat_root != nullptr) {
    fat_root->priv = new FatDirInfo{.cluster = s.is_fat32 ? s.root_cluster : 0U};
  }

  // Mount the FAT filesystem for create/mkdir/unlink/rename.
  Vfs::mount("/fat", &fat_fs_ops);

  // Recursively scan the FAT directory tree.
  if (s.is_fat32) {
    scan_directory(0, 0, s.root_cluster, "/fat");
  } else {
    scan_directory(s.root_dir_start, s.root_dir_sectors, 0, "/fat");
  }

  printf("fat: mounted %s (%u sectors/cluster, %u total clusters)\n",
         s.is_fat32 ? "FAT32" : "FAT16", s.sectors_per_cluster, s.total_clusters);
}

}  // namespace Fat
