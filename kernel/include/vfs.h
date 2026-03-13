#pragma once

#include <optional.h>
#include <span.h>
#include <stddef.h>
#include <stdint.h>

#include "file.h"

// Maximum number of VFS nodes in the system.
static constexpr uint32_t kMaxVfsNodes = 64;

// Maximum path length (including null terminator).
static constexpr uint32_t kMaxPathLen = 128;

// VFS node types.
enum class VfsNodeType : uint8_t {
  File,       // regular file (ramfs data)
  CharDev,    // character device (e.g. /dev/tty, /dev/null)
};

// Operations that a VFS node can support. Each backend (ramfs, devfs)
// provides its own implementations. Null pointers mean "not supported".
struct VfsOps {
  int32_t (*read)(struct VfsNode* node, std::span<uint8_t> buf, uint32_t offset);
  int32_t (*write)(struct VfsNode* node, std::span<const uint8_t> buf, uint32_t offset);
};

// An open-file description backed by a VFS node. Tracks the per-fd
// read/write offset for regular files.
struct VfsFileDescription {
  struct VfsNode* node;
  uint32_t offset;
};

// A single node in the VFS. Represents a file or device.
struct VfsNode {
  char name[kMaxPathLen];   // full path (e.g. "/dev/tty", "/bin/sh.elf")
  VfsNodeType type;
  const VfsOps* ops;

  // For File nodes: pointer to data and length.
  const uint8_t* data;
  size_t size;
};

namespace Vfs {

// Initialise the VFS. Must be called before any other VFS function.
void init();

// Register a new node. Returns a pointer to the node, or nullptr if the
// table is full or the name is too long.
[[nodiscard]] VfsNode* register_node(const char* path, VfsNodeType type,
                                     const VfsOps* ops);

// Look up a node by its full path. Returns nullptr if not found.
[[nodiscard]] VfsNode* lookup(const char* path);

// Open a VFS node and install it into the process's fd table.
// Returns the fd number, or -1 on failure.
[[nodiscard]] int32_t open(const char* path);

// Read from a VFS-backed file description.
[[nodiscard]] int32_t read(FileDescription* fd, std::span<uint8_t> buf);

// Write to a VFS-backed file description.
[[nodiscard]] int32_t write(FileDescription* fd, std::span<const uint8_t> buf);

// Close a VFS-backed file description. Frees the VfsFileDescription.
void close(FileDescription* fd);

// Populate ramfs nodes from multiboot modules. Each module becomes
// a file at "/bin/<basename>".
void init_ramfs();

// Initialise devfs device nodes (/dev/tty, /dev/null).
void init_devfs();

}  // namespace Vfs
