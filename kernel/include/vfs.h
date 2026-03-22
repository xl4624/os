#pragma once

#include <dirent.h>
#include <optional.h>
#include <span.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "file.h"

// Maximum path length (including null terminator).
static constexpr uint32_t kMaxPathLen = 128;

// VFS node types.
enum class VfsNodeType : uint8_t {
  File,       // regular file (ramfs data or FAT file)
  CharDev,    // character device (e.g. /dev/tty, /dev/null)
  Directory,  // directory node
};

// Operations that a VFS node can support. Each backend (ramfs, devfs, fat)
// provides its own implementations. Null pointers mean "not supported".
struct VfsOps {
  int32_t (*read)(struct VfsNode* node, std::span<uint8_t> buf, uint32_t offset);
  int32_t (*write)(struct VfsNode* node, std::span<const uint8_t> buf, uint32_t offset);
  int32_t (*ioctl)(struct VfsNode* node, uint32_t request, void* arg);
  int32_t (*truncate)(struct VfsNode* node);
};

// An open-file description backed by a VFS node. Tracks the per-fd
// read/write offset for regular files.
struct VfsFileDescription {
  struct VfsNode* node;
  uint32_t offset;
  int32_t open_flags;
};

// A single node in the VFS. Represents a file, device, or directory.
struct VfsNode {
  char name[kMaxPathLen];  // full path (e.g. "/dev/tty", "/bin/sh")
  VfsNodeType type;
  const VfsOps* ops;

  // For ramfs File nodes: pointer to immutable data and length.
  const uint8_t* data;
  size_t size;

  // For mutable FAT nodes: per-file mutable metadata (FatFileInfo* or FatDirInfo*).
  void* priv;
};

// Filesystem-level operations for a mounted volume (mkdir, unlink, etc.).
struct FsOps {
  int32_t (*mkdir)(const char* abs_path);
  int32_t (*unlink)(const char* abs_path);
  int32_t (*rename)(const char* old_path, const char* new_path);
  VfsNode* (*create)(const char* abs_path, int32_t flags, mode_t mode);
};

// Forward declaration for stat_path.
struct stat;

namespace Vfs {

// Initialise the VFS. Must be called before any other VFS function.
void init();

// Register a new node. Returns a pointer to the node, or nullptr if
// the name is too long.
[[nodiscard]] VfsNode* register_node(const char* path, VfsNodeType type, const VfsOps* ops);

// Soft-delete a node by path (clears its name so it is skipped by lookup).
void unregister_node(const char* path);

// Look up a node by its full path. Returns nullptr if not found.
[[nodiscard]] VfsNode* lookup(const char* path);

// Open a VFS node and install it into the process's fd table.
// flags uses the same O_* constants as open(2). If O_CREAT is set and the
// node does not exist, a filesystem create is attempted.
// Returns the fd number, or negative errno on failure.
[[nodiscard]] int32_t open(const char* path, int32_t flags, mode_t mode = 0);

// Read from a VFS-backed file description.
[[nodiscard]] int32_t read(FileDescription* fd, std::span<uint8_t> buf);

// Write to a VFS-backed file description.
[[nodiscard]] int32_t write(FileDescription* fd, std::span<const uint8_t> buf);

// Perform a device-specific control operation on a VFS-backed file description.
[[nodiscard]] int32_t ioctl(FileDescription* fd, uint32_t request, void* arg);

// Close a VFS-backed file description. Frees the VfsFileDescription.
void close(FileDescription* fd);

// Returns true if path names a valid virtual directory.
[[nodiscard]] bool is_directory(const char* path);

// Fill entries with the direct children of path. Returns the number of
// entries written, or 0 if path is not a directory or is empty.
uint32_t getdents(const char* path, dirent* entries, uint32_t max_entries);

// Register a filesystem at a mount prefix (e.g. "/fat").
void mount(const char* prefix, const FsOps* ops);

// Filesystem-level operations -- dispatched to the matching mount.
int32_t fs_mkdir(const char* abs_path);
int32_t fs_unlink(const char* abs_path);
int32_t fs_rename(const char* old_path, const char* new_path);

// Fill buf with stat information for the given path.
// Returns 0 on success, or negative errno on failure.
int32_t stat_path(const char* path, struct stat* buf);

// Populate ramfs nodes from multiboot modules. Each module becomes
// a file at "/bin/<basename>".
void init_ramfs();

// Initialise devfs device nodes (/dev/tty, /dev/null, /dev/kbd, /dev/fb).
void init_devfs();

}  // namespace Vfs
