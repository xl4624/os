#include "vfs.h"

#include <algorithm.h>
#include <string.h>
#include <sys/fb.h>

#include "framebuffer.h"
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
  const size_t n = kKeyboard.read(cbuf, buf.size());
  return static_cast<int32_t>(n);
}

int32_t tty_write([[maybe_unused]] VfsNode* node, std::span<const uint8_t> buf,
                  [[maybe_unused]] uint32_t offset) {
  terminal_write({reinterpret_cast<const char*>(buf.data()), buf.size()});
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

int32_t kbd_read([[maybe_unused]] VfsNode* node, std::span<uint8_t> buf,
                 [[maybe_unused]] uint32_t offset) {
  const size_t max_events = buf.size() / sizeof(kbd_event);
  if (max_events == 0) {
    return 0;
  }
  auto* events = reinterpret_cast<kbd_event*>(buf.data());
  const size_t n = kKeyboard.read_events(events, max_events);
  return static_cast<int32_t>(n * sizeof(kbd_event));
}

int32_t kbd_write([[maybe_unused]] VfsNode* node, [[maybe_unused]] std::span<const uint8_t> buf,
                  [[maybe_unused]] uint32_t offset) {
  return -1;  // not writable
}

const VfsOps kbd_ops = {kbd_read, kbd_write};

// ===========================================================================
// framebuffer device operations
// ===========================================================================

// Reading /dev/fb at offset 0 returns an fb_info struct, followed by raw
// framebuffer pixel data starting at offset sizeof(fb_info).
int32_t fb_read([[maybe_unused]] VfsNode* node, std::span<uint8_t> buf, uint32_t offset) {
  if (!Framebuffer::is_available()) {
    return -1;
  }

  const auto& fi = Framebuffer::info();
  const size_t fb_sz = Framebuffer::size();

  // Build the info header.
  fb_info header;
  header.width = fi.width;
  header.height = fi.height;
  header.pitch = fi.pitch;
  header.size = static_cast<uint32_t>(fb_sz);
  header.bpp = fi.bpp;
  header.red_pos = fi.red_pos;
  header.red_size = fi.red_size;
  header.green_pos = fi.green_pos;
  header.green_size = fi.green_size;
  header.blue_pos = fi.blue_pos;
  header.blue_size = fi.blue_size;
  header.reserved = 0;

  // Total virtual size: header + framebuffer pixels.
  const size_t total = sizeof(fb_info) + fb_sz;
  if (offset >= total) {
    return 0;
  }

  const size_t remaining = total - offset;
  size_t to_read = buf.size();
  to_read = std::min(to_read, remaining);

  // Copy from the concatenation of [header][framebuffer].
  size_t copied = 0;
  if (offset < sizeof(fb_info)) {
    size_t hdr_bytes = sizeof(fb_info) - offset;
    hdr_bytes = std::min(hdr_bytes, to_read);
    memcpy(buf.data(), reinterpret_cast<const uint8_t*>(&header) + offset, hdr_bytes);
    copied += hdr_bytes;
  }

  if (copied < to_read) {
    const size_t fb_offset = (offset > sizeof(fb_info)) ? offset - sizeof(fb_info) : 0;
    const size_t fb_bytes = to_read - copied;
    memcpy(buf.data() + copied, Framebuffer::buffer() + fb_offset, fb_bytes);
    copied += fb_bytes;
  }

  return static_cast<int32_t>(copied);
}

// Writing to /dev/fb copies raw pixel data into the framebuffer at offset.
int32_t fb_write([[maybe_unused]] VfsNode* node, std::span<const uint8_t> buf, uint32_t offset) {
  if (!Framebuffer::is_available()) {
    return -1;
  }

  const size_t fb_sz = Framebuffer::size();
  if (offset >= fb_sz) {
    return 0;
  }

  const size_t remaining = fb_sz - offset;
  size_t to_write = buf.size();
  to_write = std::min(to_write, remaining);

  memcpy(Framebuffer::buffer() + offset, buf.data(), to_write);
  return static_cast<int32_t>(to_write);
}

const VfsOps fb_ops = {fb_read, fb_write};

// ===========================================================================
// ramfs operations
// ===========================================================================

int32_t ramfs_read(VfsNode* node, std::span<uint8_t> buf, uint32_t offset) {
  if (offset >= node->size) {
    return 0;  // EOF
  }
  const size_t remaining = node->size - offset;
  size_t to_read = buf.size();
  to_read = std::min(to_read, remaining);
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
  if (node == nullptr) {
    return -1;
  }

  Process* proc = Scheduler::current();
  if (proc == nullptr) {
    return -1;
  }

  auto slot = fd_alloc(proc->fds);
  if (!slot) {
    return -1;
  }

  auto* vfs_fd = new VfsFileDescription{node, 0};
  if (vfs_fd == nullptr) {
    return -1;
  }

  auto* desc = new FileDescription{FileType::VfsNode, 1, nullptr, vfs_fd};
  if (desc == nullptr) {
    delete vfs_fd;
    return -1;
  }

  proc->fds[*slot] = desc;
  return static_cast<int32_t>(*slot);
}

int32_t read(FileDescription* fd, std::span<uint8_t> buf) {
  auto* vfs_fd = fd->vfs;
  if ((vfs_fd == nullptr) || (vfs_fd->node == nullptr) || (vfs_fd->node->ops == nullptr) ||
      (vfs_fd->node->ops->read == nullptr)) {
    return -1;
  }

  const int32_t n = vfs_fd->node->ops->read(vfs_fd->node, buf, vfs_fd->offset);
  if (n > 0) {
    vfs_fd->offset += static_cast<uint32_t>(n);
  }
  return n;
}

int32_t write(FileDescription* fd, std::span<const uint8_t> buf) {
  auto* vfs_fd = fd->vfs;
  if ((vfs_fd == nullptr) || (vfs_fd->node == nullptr) || (vfs_fd->node->ops == nullptr) ||
      (vfs_fd->node->ops->write == nullptr)) {
    return -1;
  }

  const int32_t n = vfs_fd->node->ops->write(vfs_fd->node, buf, vfs_fd->offset);
  if (n > 0) {
    vfs_fd->offset += static_cast<uint32_t>(n);
  }
  return n;
}

void close(FileDescription* fd) {
  if (fd->vfs != nullptr) {
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

  auto* kbd = register_node("/dev/kbd", VfsNodeType::CharDev, &kbd_ops);
  (void)kbd;

  if (Framebuffer::is_available()) {
    auto* fb = register_node("/dev/fb", VfsNodeType::CharDev, &fb_ops);
    (void)fb;
  }
}

void init_ramfs() {
  for (uint32_t i = 0; i < Modules::count(); ++i) {
    const Module* mod = Modules::get(i);
    if (mod == nullptr) {
      continue;
    }

    const size_t name_len = strlen(mod->name);
    if (name_len + 6 > kMaxPathLen) {
      continue;
    }

    // Strip ".elf" suffix if present; detect whether this is an ELF module.
    char name[kMaxPathLen];
    strcpy(name, mod->name);
    const bool is_elf = (name_len > 4 && strcmp(name + name_len - 4, ".elf") == 0);
    if (is_elf) {
      name[name_len - 4] = '\0';
    }

    // All modules go into /bin/<name>.
    char path[kMaxPathLen];
    strcpy(path, "/bin/");
    strcpy(path + 5, name);

    auto* node = register_node(path, VfsNodeType::File, &ramfs_ops);
    if (node != nullptr) {
      node->data = mod->data;
      node->size = mod->len;
    }

    // Non-ELF modules (e.g. .wad data files) are also accessible at /<name>
    // so programs can open them with an absolute path like "/doom1.wad".
    if (!is_elf) {
      char root_path[kMaxPathLen];
      root_path[0] = '/';
      strcpy(root_path + 1, name);

      auto* root_node = register_node(root_path, VfsNodeType::File, &ramfs_ops);
      if (root_node != nullptr) {
        root_node->data = mod->data;
        root_node->size = mod->len;
      }
    }
  }
}

}  // namespace Vfs
