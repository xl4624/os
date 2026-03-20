#include "fat16.h"

#include <algorithm.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ata.h"
#include "heap.h"
#include "vfs.h"

// FAT16 Boot Parameter Block (first 62 bytes of the boot sector).
struct Fat16Bpb {
  uint8_t jump_boot[3];
  char oem_name[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sector_count;
  uint8_t num_fats;
  uint16_t root_entry_count;
  uint16_t total_sectors_16;
  uint8_t media_type;
  uint16_t fat_size_16;  // sectors per FAT (FAT16 only; 0 on FAT32)
  uint16_t sectors_per_track;
  uint16_t num_heads;
  uint32_t hidden_sectors;
  uint32_t total_sectors_32;
  // FAT16-specific extension (offset 36)
  uint8_t drive_number;
  uint8_t reserved1;
  uint8_t boot_sig;
  uint32_t volume_id;
  char volume_label[11];
  char fs_type[8];  // "FAT16   "
} __attribute__((packed));
static_assert(sizeof(Fat16Bpb) == 62, "Fat16Bpb size mismatch");

// A 32-byte directory entry as stored on disk.
struct Fat16DirEntry {
  char name[8];  // 8.3 base name, space-padded
  char ext[3];   // 8.3 extension, space-padded
  uint8_t attr;
  uint8_t reserved[10];
  uint16_t write_time;
  uint16_t write_date;
  uint16_t first_cluster;
  uint32_t file_size;
} __attribute__((packed));
static_assert(sizeof(Fat16DirEntry) == 32, "Fat16DirEntry size mismatch");

// Directory entry attribute flags.
static constexpr uint8_t kAttrHidden = 0x02;
static constexpr uint8_t kAttrSystem = 0x04;
static constexpr uint8_t kAttrVolumeId = 0x08;
static constexpr uint8_t kAttrDirectory = 0x10;
static constexpr uint8_t kAttrLfnMask = 0x0F;  // all four low bits set = LFN entry

// FAT16 cluster chain sentinel: any value >= this is end-of-chain.
static constexpr uint16_t kClusterEoc = 0xFFF8;

// Per-file metadata stored in VfsNode::data for FAT16 nodes.
struct Fat16FileInfo {
  uint16_t start_cluster;
  uint32_t file_size;
};

namespace {

struct Fat16State {
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint32_t fat_start;       // first sector of FAT1
  uint32_t root_dir_start;  // first sector of the root directory region
  uint32_t root_dir_sectors;
  uint32_t data_start;  // first sector of the data region (cluster 2)
  uint16_t root_entry_count;
  uint8_t* fat;  // heap-allocated copy of FAT1
  bool mounted;
};

Fat16State s_state = {};

uint16_t fat_entry(uint16_t cluster) {
  assert(s_state.fat != nullptr);
  const uint32_t byte_off = static_cast<uint32_t>(cluster) * 2;
  return *reinterpret_cast<const uint16_t*>(s_state.fat + byte_off);
}

// Convert an 8.3 name/ext pair into a lowercase null-terminated string.
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

int32_t fat16_read(VfsNode* node, std::span<uint8_t> buf, uint32_t offset) {
  assert(s_state.mounted);
  const auto* fi = reinterpret_cast<const Fat16FileInfo*>(node->data);
  assert(fi != nullptr);

  if (offset >= fi->file_size) {
    return 0;
  }

  const uint32_t bps = s_state.bytes_per_sector;
  const uint32_t spc = s_state.sectors_per_cluster;
  const uint32_t bytes_per_cluster = bps * spc;

  const auto buf_len = static_cast<uint32_t>(buf.size());
  const uint32_t available = fi->file_size - offset;
  uint32_t remaining = std::min(buf_len, available);
  uint32_t out_pos = 0;

  // Walk the cluster chain to the cluster containing 'offset'.
  const uint32_t cluster_idx = offset / bytes_per_cluster;
  uint32_t cluster_off = offset % bytes_per_cluster;

  uint16_t cluster = fi->start_cluster;
  for (uint32_t i = 0; i < cluster_idx; ++i) {
    cluster = fat_entry(cluster);
    if (cluster >= kClusterEoc) {
      return 0;
    }
  }

  uint8_t sector_buf[512];

  while (remaining > 0 && cluster < kClusterEoc) {
    const uint32_t clus_sector = s_state.data_start + (static_cast<uint32_t>(cluster - 2) * spc);

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
    cluster = fat_entry(cluster);
  }

  return static_cast<int32_t>(out_pos);
}

int32_t fat16_write([[maybe_unused]] VfsNode* node, [[maybe_unused]] std::span<const uint8_t> buf,
                    [[maybe_unused]] uint32_t offset) {
  // TODO: implement FAT16 writes
  return -1;
}

const VfsOps fat16_ops = {fat16_read, fat16_write};

}  // namespace

namespace Fat16 {

void init_vfs() {
  if (!Ata::is_present()) {
    return;
  }

  uint8_t boot_sector[512];
  if (!Ata::read_sector(0, boot_sector)) {
    printf("fat16: failed to read boot sector\n");
    return;
  }

  const auto* bpb = reinterpret_cast<const Fat16Bpb*>(boot_sector);

  if (bpb->bytes_per_sector != 512) {
    printf("fat16: unsupported sector size %u\n", bpb->bytes_per_sector);
    return;
  }
  if (bpb->fat_size_16 == 0) {
    printf("fat16: fat_size_16 == 0 (FAT32 or corrupt BPB)\n");
    return;
  }
  if (bpb->sectors_per_cluster == 0) {
    printf("fat16: sectors_per_cluster == 0 (corrupt BPB)\n");
    return;
  }

  s_state.bytes_per_sector = bpb->bytes_per_sector;
  s_state.sectors_per_cluster = bpb->sectors_per_cluster;
  s_state.root_entry_count = bpb->root_entry_count;

  s_state.fat_start = bpb->reserved_sector_count;
  s_state.root_dir_start =
      s_state.fat_start + (static_cast<uint32_t>(bpb->num_fats) * bpb->fat_size_16);

  const uint32_t root_bytes = static_cast<uint32_t>(bpb->root_entry_count) * sizeof(Fat16DirEntry);
  s_state.root_dir_sectors = (root_bytes + 511) / 512;
  s_state.data_start = s_state.root_dir_start + s_state.root_dir_sectors;

  // Cache FAT1 to avoid repeated sector reads on every file access.
  const uint32_t fat_bytes = static_cast<uint32_t>(bpb->fat_size_16) * 512;
  s_state.fat = new uint8_t[fat_bytes];
  if (s_state.fat == nullptr) {
    printf("fat16: out of memory caching FAT\n");
    return;
  }

  for (uint32_t s = 0; s < bpb->fat_size_16; ++s) {
    if (!Ata::read_sector(s_state.fat_start + s, s_state.fat + (s * 512))) {
      printf("fat16: failed to read FAT sector %u\n", s);
      delete[] s_state.fat;
      s_state.fat = nullptr;
      return;
    }
  }

  s_state.mounted = true;

  // Scan the root directory and register each file as a VFS node at /fat/<name>.
  // TODO: recurse into subdirectories.
  uint8_t dir_sector[512];
  static constexpr uint32_t kEntriesPerSector = 512 / sizeof(Fat16DirEntry);

  for (uint32_t s = 0; s < s_state.root_dir_sectors; ++s) {
    if (!Ata::read_sector(s_state.root_dir_start + s, dir_sector)) {
      break;
    }

    const auto* entries = reinterpret_cast<const Fat16DirEntry*>(dir_sector);

    for (uint32_t e = 0; e < kEntriesPerSector; ++e) {
      const Fat16DirEntry& de = entries[e];

      if (static_cast<uint8_t>(de.name[0]) == 0x00) {
        goto done;  // end of directory
      }
      if (static_cast<uint8_t>(de.name[0]) == 0xE5) {
        continue;  // deleted entry
      }

      // Skip volume label, directories, LFN entries, and hidden/system files.
      if ((de.attr & kAttrVolumeId) != 0) {
        continue;
      }
      if ((de.attr & kAttrDirectory) != 0) {
        continue;
      }
      if ((de.attr & kAttrLfnMask) == kAttrLfnMask) {
        continue;
      }
      if ((de.attr & (kAttrHidden | kAttrSystem)) != 0) {
        continue;
      }

      char fname[13];  // 8 + '.' + 3 + NUL
      format_name(de.name, de.ext, fname, sizeof(fname));

      char path[kMaxPathLen];
      memcpy(path, "/fat/", 5);
      strncpy(path + 5, fname, kMaxPathLen - 6);
      path[kMaxPathLen - 1] = '\0';

      VfsNode* node = Vfs::register_node(path, VfsNodeType::File, &fat16_ops);
      if (node == nullptr) {
        continue;
      }

      auto* fi = new Fat16FileInfo{de.first_cluster, de.file_size};
      node->data = reinterpret_cast<const uint8_t*>(fi);
      node->size = de.file_size;
    }
  }

done:;
  printf("fat16: mounted (%u sectors/cluster, %u root entries)\n", s_state.sectors_per_cluster,
         s_state.root_entry_count);
}

}  // namespace Fat16
