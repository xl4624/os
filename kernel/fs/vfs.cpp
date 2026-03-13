#include "vfs.h"

#include <string.h>

#include "keyboard.h"
#include "modules.h"
#include "scheduler.h"
#include "tty.h"

namespace {

uint32_t node_count = 0;
VfsNode node_table[kMaxVfsNodes];

// ===========================================================================
// devfs operations
// ===========================================================================

int32_t tty_read([[maybe_unused]] VfsNode* node, std::span<uint8_t> buf,
                 [[maybe_unused]] uint32_t offset) {
  char* cbuf = reinterpret_cast<char*>(buf.data());
  size_t n = kKeyboard.read(cbuf, buf.size());
  return static_cast<int32_t>(n);
}

int32_t tty_write([[maybe_unused]] VfsNode* node, std::span<const uint8_t> buf,
                  [[maybe_unused]] uint32_t offset) {
  const char* data = reinterpret_cast<const char*>(buf.data());
  for (size_t i = 0; i < buf.size(); ++i) {
    terminal_putchar(data[i]);
  }
  return static_cast<int32_t>(buf.size());
}

const VfsOps tty_ops = {tty_read, tty_write};

int32_t null_read([[maybe_unused]] VfsNode* node, [[maybe_unused]] std::span<uint8_t> buf,
                  [[maybe_unused]] uint32_t offset) {
  return 0;  // EOF
}

int32_t null_write([[maybe_unused]] VfsNode* node, std::span<const uint8_t> buf,
                   [[maybe_unused]] uint32_t offset) {
  return static_cast<int32_t>(buf.size());  // discard
}

const VfsOps null_ops = {null_read, null_write};

// ===========================================================================
// ramfs operations
// ===========================================================================

int32_t ramfs_read(VfsNode* node, std::span<uint8_t> buf, uint32_t offset) {
  if (offset >= node->size) {
    return 0;  // EOF
  }
  size_t remaining = node->size - offset;
  size_t to_read = buf.size();
  if (to_read > remaining) {
    to_read = remaining;
  }
  memcpy(buf.data(), node->data + offset, to_read);
  return static_cast<int32_t>(to_read);
}

int32_t ramfs_write([[maybe_unused]] VfsNode* node, [[maybe_unused]] std::span<const uint8_t> buf,
                    [[maybe_unused]] uint32_t offset) {
  return -1;  // read-only
}

const VfsOps ramfs_ops = {ramfs_read, ramfs_write};

}  // namespace

namespace Vfs {

void init() {
  node_count = 0;
  memset(node_table, 0, sizeof(node_table));
}

VfsNode* register_node(const char* path, VfsNodeType type, const VfsOps* ops) {
  if (node_count >= kMaxVfsNodes) {
    return nullptr;
  }
  if (strlen(path) >= kMaxPathLen) {
    return nullptr;
  }

  VfsNode* node = &node_table[node_count];
  strcpy(node->name, path);
  node->type = type;
  node->ops = ops;
  node->data = nullptr;
  node->size = 0;
  ++node_count;
  return node;
}

VfsNode* lookup(const char* path) {
  for (uint32_t i = 0; i < node_count; ++i) {
    if (strcmp(node_table[i].name, path) == 0) {
      return &node_table[i];
    }
  }
  return nullptr;
}

int32_t open(const char* path) {
  VfsNode* node = lookup(path);
  if (!node) {
    return -1;
  }

  Process* proc = Scheduler::current();
  if (!proc) {
    return -1;
  }

  auto slot = fd_alloc(proc->fds);
  if (!slot) {
    return -1;
  }

  auto* vfs_fd = new VfsFileDescription{node, 0};
  if (!vfs_fd) {
    return -1;
  }

  auto* desc = new FileDescription{FileType::VfsNode, 1, nullptr, vfs_fd};
  if (!desc) {
    delete vfs_fd;
    return -1;
  }

  proc->fds[*slot] = desc;
  return static_cast<int32_t>(*slot);
}

int32_t read(FileDescription* fd, std::span<uint8_t> buf) {
  auto* vfs_fd = fd->vfs;
  if (!vfs_fd || !vfs_fd->node || !vfs_fd->node->ops || !vfs_fd->node->ops->read) {
    return -1;
  }

  int32_t n = vfs_fd->node->ops->read(vfs_fd->node, buf, vfs_fd->offset);
  if (n > 0 && vfs_fd->node->type == VfsNodeType::File) {
    vfs_fd->offset += static_cast<uint32_t>(n);
  }
  return n;
}

int32_t write(FileDescription* fd, std::span<const uint8_t> buf) {
  auto* vfs_fd = fd->vfs;
  if (!vfs_fd || !vfs_fd->node || !vfs_fd->node->ops || !vfs_fd->node->ops->write) {
    return -1;
  }

  int32_t n = vfs_fd->node->ops->write(vfs_fd->node, buf, vfs_fd->offset);
  if (n > 0 && vfs_fd->node->type == VfsNodeType::File) {
    vfs_fd->offset += static_cast<uint32_t>(n);
  }
  return n;
}

void close(FileDescription* fd) {
  if (fd->vfs) {
    delete fd->vfs;
    fd->vfs = nullptr;
  }
  delete fd;
}

void init_devfs() {
  auto* tty = register_node("/dev/tty", VfsNodeType::CharDev, &tty_ops);
  (void)tty;

  auto* null = register_node("/dev/null", VfsNodeType::CharDev, &null_ops);
  (void)null;
}

void init_ramfs() {
  for (uint32_t i = 0; i < Modules::count(); ++i) {
    const Module* mod = Modules::get(i);
    if (!mod) {
      continue;
    }

    // Build path "/bin/<name>"
    char path[kMaxPathLen];
    // Ensure path fits: "/bin/" (5) + name + '\0'
    size_t name_len = strlen(mod->name);
    if (name_len + 6 > kMaxPathLen) {
      continue;
    }
    strcpy(path, "/bin/");
    strcpy(path + 5, mod->name);

    auto* node = register_node(path, VfsNodeType::File, &ramfs_ops);
    if (node) {
      node->data = mod->data;
      node->size = mod->len;
    }
  }
}

}  // namespace Vfs
