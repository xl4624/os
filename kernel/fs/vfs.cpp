#include "vfs.h"

#include <algorithm.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/fb.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <vector.h>

#include "framebuffer.h"
#include "keyboard.h"
#include "modules.h"
#include "scheduler.h"
#include "tty.h"

namespace {

std::vector<VfsNode> node_table;

// ===========================================================================
// Mount registry
// ===========================================================================

struct MountEntry {
  char prefix[kMaxPathLen];
  const FsOps* ops;
};

constexpr uint32_t kMaxMounts = 8;
MountEntry mount_table[kMaxMounts];
uint32_t mount_count = 0;

// ===========================================================================
// TTY state (termios + ICANON line buffer)
// ===========================================================================

struct TtyState {
  struct termios termios;
  char line_buf[256];
  size_t line_len;
  size_t line_pos;  // bytes already handed to the caller
};

TtyState tty_state = {
    .termios = {.c_iflag = 0,
                .c_oflag = 0,
                .c_cflag = 0,
                .c_lflag = ICANON | ECHO | ECHOE | ISIG,
                .c_cc = {}},
    .line_buf = {},
    .line_len = 0,
    .line_pos = 0,
};

// ===========================================================================
// devfs operations
// ===========================================================================

int32_t tty_read([[maybe_unused]] VfsNode* node, std::span<uint8_t> buf,
                 [[maybe_unused]] uint32_t offset) {
  // Raw mode: return whatever is in the keyboard buffer immediately.
  if ((tty_state.termios.c_lflag & ICANON) == 0U) {
    char* cbuf = reinterpret_cast<char*>(buf.data());
    const size_t n = kKeyboard.read(cbuf, buf.size());
    return static_cast<int32_t>(n);
  }

  // Canonical mode: accumulate into line_buf until '\n', then serve line.
  // If there is already buffered data from a previous '\n', hand it out.
  if (tty_state.line_pos < tty_state.line_len) {
    const size_t avail = tty_state.line_len - tty_state.line_pos;
    const size_t to_copy = avail < buf.size() ? avail : buf.size();
    memcpy(buf.data(), tty_state.line_buf + tty_state.line_pos, to_copy);
    tty_state.line_pos += to_copy;
    if (tty_state.line_pos >= tty_state.line_len) {
      tty_state.line_len = 0;
      tty_state.line_pos = 0;
    }
    return static_cast<int32_t>(to_copy);
  }

  // Read new characters from keyboard into the line buffer.
  char c;
  while (kKeyboard.read(&c, 1) == 1) {
    if (c == '\b' || c == 127) {
      if (tty_state.line_len > 0) {
        --tty_state.line_len;
        if ((tty_state.termios.c_lflag & ECHOE) != 0U) {
          terminal_write("\b \b");
        }
      }
      continue;
    }
    if ((tty_state.termios.c_lflag & ECHO) != 0U) {
      const char echo_str[2] = {(c == '\r') ? '\n' : c, '\0'};
      terminal_write(echo_str);
    }
    const char store = (c == '\r') ? '\n' : c;
    if (tty_state.line_len < sizeof(tty_state.line_buf) - 1) {
      tty_state.line_buf[tty_state.line_len++] = store;
    }
    if (store == '\n') {
      // Line complete: serve it.
      const size_t to_copy = tty_state.line_len < buf.size() ? tty_state.line_len : buf.size();
      memcpy(buf.data(), tty_state.line_buf, to_copy);
      tty_state.line_pos = to_copy;
      if (tty_state.line_pos >= tty_state.line_len) {
        tty_state.line_len = 0;
        tty_state.line_pos = 0;
      }
      return static_cast<int32_t>(to_copy);
    }
  }

  // No newline yet; ask the scheduler to retry.
  return 0;
}

int32_t tty_ioctl([[maybe_unused]] VfsNode* node, uint32_t request, void* arg) {
  switch (request) {
    case TIOCGWINSZ: {
      auto* ws = static_cast<struct winsize*>(arg);
      ws->ws_row = 25;
      ws->ws_col = 80;
      ws->ws_xpixel = 0;
      ws->ws_ypixel = 0;
      return 0;
    }
    case TCGETS: {
      auto* t = static_cast<struct termios*>(arg);
      *t = tty_state.termios;
      return 0;
    }
    case TCSETS: {
      const auto* t = static_cast<const struct termios*>(arg);
      tty_state.termios = *t;
      // TCSAFLUSH semantics: discard pending input.
      tty_state.line_len = 0;
      tty_state.line_pos = 0;
      return 0;
    }
    default:
      return -ENOTTY;
  }
}

int32_t tty_write([[maybe_unused]] VfsNode* node, std::span<const uint8_t> buf,
                  [[maybe_unused]] uint32_t offset) {
  terminal_write({reinterpret_cast<const char*>(buf.data()), buf.size()});
  return static_cast<int32_t>(buf.size());
}

const VfsOps tty_ops = {
    .read = tty_read, .write = tty_write, .ioctl = tty_ioctl, .truncate = nullptr};

int32_t null_read([[maybe_unused]] VfsNode* node, [[maybe_unused]] std::span<uint8_t> buf,
                  [[maybe_unused]] uint32_t offset) {
  return 0;  // EOF
}

int32_t null_write([[maybe_unused]] VfsNode* node, std::span<const uint8_t> buf,
                   [[maybe_unused]] uint32_t offset) {
  return static_cast<int32_t>(buf.size());  // discard
}

const VfsOps null_ops = {
    .read = null_read, .write = null_write, .ioctl = nullptr, .truncate = nullptr};

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

const VfsOps kbd_ops = {
    .read = kbd_read, .write = kbd_write, .ioctl = nullptr, .truncate = nullptr};

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

const VfsOps fb_ops = {.read = fb_read, .write = fb_write, .ioctl = nullptr, .truncate = nullptr};

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

const VfsOps ramfs_ops = {
    .read = ramfs_read, .write = ramfs_write, .ioctl = nullptr, .truncate = nullptr};

}  // namespace

namespace Vfs {

void init() {
  node_table.clear();
  node_table.reserve(256);
  mount_count = 0;
}

VfsNode* register_node(const char* path, VfsNodeType type, const VfsOps* ops) {
  assert(path != nullptr && path[0] == '/' &&
         "VfsNode::register_node: path must be non-null and start with '/'");
  if (strlen(path) >= kMaxPathLen) {
    return nullptr;
  }

  node_table.push_back({});
  VfsNode* node = &node_table.back();
  strncpy(node->name, path, kMaxPathLen - 1);
  node->name[kMaxPathLen - 1] = '\0';
  node->type = type;
  node->ops = ops;
  node->data = nullptr;
  node->size = 0;
  node->priv = nullptr;
  return node;
}

void unregister_node(const char* path) {
  assert(path != nullptr && "unregister_node(): null path");
  for (auto& node : node_table) {
    if (strcmp(node.name, path) == 0) {
      node.name[0] = '\0';  // soft-delete: skip in lookup/getdents
      return;
    }
  }
}

VfsNode* lookup(const char* path) {
  assert(path != nullptr && path[0] == '/' && "lookup(): path must be non-null and absolute");
  for (auto& node : node_table) {
    if (node.name[0] != '\0' && strcmp(node.name, path) == 0) {
      return &node;
    }
  }
  return nullptr;
}

int32_t open(const char* path, int32_t flags, mode_t mode) {
  assert(path != nullptr && path[0] == '/' && "open(): path must be non-null and absolute");

  VfsNode* node = lookup(path);

  // O_CREAT: try to create the file if it does not exist.
  if (node == nullptr && (flags & O_CREAT) != 0) {
    for (uint32_t i = 0; i < mount_count; ++i) {
      const size_t plen = strlen(mount_table[i].prefix);
      if (strncmp(path, mount_table[i].prefix, plen) == 0 &&
          (path[plen] == '/' || path[plen] == '\0') && mount_table[i].ops->create != nullptr) {
        node = mount_table[i].ops->create(path, flags, mode);
        break;
      }
    }
  }

  if (node == nullptr) {
    return -1;
  }

  // O_TRUNC: truncate an existing writable file.
  if ((flags & O_TRUNC) != 0 && node->type == VfsNodeType::File && node->ops != nullptr &&
      node->ops->truncate != nullptr) {
    node->ops->truncate(node);
  }

  Process* proc = Scheduler::current();
  if (proc == nullptr) {
    return -1;
  }

  auto slot = fd_alloc(proc->fds);
  if (!slot) {
    return -1;
  }

  auto* vfs_fd = new VfsFileDescription{.node = node, .offset = 0, .open_flags = flags};
  if (vfs_fd == nullptr) {
    return -1;
  }

  auto* desc = new FileDescription{
      .type = FileType::VfsNode, .ref_count = 1, .pipe = nullptr, .vfs = vfs_fd};
  if (desc == nullptr) {
    delete vfs_fd;
    return -1;
  }

  proc->fds[*slot] = desc;
  return static_cast<int32_t>(*slot);
}

int32_t read(FileDescription* fd, std::span<uint8_t> buf) {
  assert(fd != nullptr && "read(): null file description");
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
  assert(fd != nullptr && "write(): null file description");
  auto* vfs_fd = fd->vfs;
  if ((vfs_fd == nullptr) || (vfs_fd->node == nullptr) || (vfs_fd->node->ops == nullptr) ||
      (vfs_fd->node->ops->write == nullptr)) {
    return -1;
  }

  // O_APPEND: seek to end before each write.
  if ((vfs_fd->open_flags & O_APPEND) != 0) {
    vfs_fd->offset = static_cast<uint32_t>(vfs_fd->node->size);
  }

  const int32_t n = vfs_fd->node->ops->write(vfs_fd->node, buf, vfs_fd->offset);
  if (n > 0) {
    vfs_fd->offset += static_cast<uint32_t>(n);
  }
  return n;
}

void close(FileDescription* fd) {
  assert(fd != nullptr && "close(): null file description");
  if (fd->vfs != nullptr) {
    delete fd->vfs;
    fd->vfs = nullptr;
  }
  delete fd;
}

bool is_directory(const char* path) {
  assert(path != nullptr && path[0] == '/' && "is_directory(): path must be non-null and absolute");
  if (path[0] != '/') {
    return false;
  }
  // Root is always a valid directory.
  if (path[1] == '\0') {
    return true;
  }
  // Strip trailing slash for comparison.
  char norm[kMaxPathLen];
  strncpy(norm, path, kMaxPathLen - 1);
  norm[kMaxPathLen - 1] = '\0';
  const size_t len = strlen(norm);
  if (len > 1 && norm[len - 1] == '/') {
    norm[len - 1] = '\0';
  }

  // An exact-match Directory node qualifies.
  for (const auto& node : node_table) {
    if (node.name[0] == '\0') {
      continue;
    }
    if (node.type == VfsNodeType::Directory && strcmp(node.name, norm) == 0) {
      return true;
    }
  }

  // Build the prefix that a child node would start with.
  char prefix[kMaxPathLen];
  strncpy(prefix, norm, kMaxPathLen - 2);
  prefix[kMaxPathLen - 2] = '\0';
  const size_t prefix_len = strlen(prefix);
  prefix[prefix_len] = '/';
  prefix[prefix_len + 1] = '\0';

  for (const auto& node : node_table) {
    if (node.name[0] == '\0') {
      continue;
    }
    if (strncmp(node.name, prefix, prefix_len + 1) == 0) {
      return true;
    }
  }
  return false;
}

uint32_t getdents(const char* path, dirent* entries, uint32_t max_entries) {
  assert(path != nullptr && path[0] == '/' && "getdents(): path must be non-null and absolute");
  assert(entries != nullptr && "getdents(): null entries buffer");
  if (max_entries == 0) {
    return 0;
  }

  char norm[kMaxPathLen];
  strncpy(norm, path, kMaxPathLen - 1);
  norm[kMaxPathLen - 1] = '\0';
  const size_t norm_len_raw = strlen(norm);
  if (norm_len_raw > 1 && norm[norm_len_raw - 1] == '/') {
    norm[norm_len_raw - 1] = '\0';
  }

  uint32_t count = 0;

  if (norm[1] == '\0') {
    // Root: collect unique first path components.
    char seen[32][32];
    uint32_t seen_count = 0;

    for (const auto& tnode : node_table) {
      if (count >= max_entries) {
        break;
      }
      if (tnode.name[0] == '\0') {
        continue;
      }
      const char* name = tnode.name;
      if (name[0] != '/') {
        continue;
      }

      const char* start = name + 1;
      const char* slash = strchr(start, '/');

      char component[128];
      bool is_dir_entry;
      if (slash != nullptr) {
        const auto clen = static_cast<size_t>(slash - start);
        if (clen == 0 || clen >= sizeof(component)) {
          continue;
        }
        strncpy(component, start, clen);
        component[clen] = '\0';
        is_dir_entry = true;
      } else {
        strncpy(component, start, sizeof(component) - 1);
        component[sizeof(component) - 1] = '\0';
        is_dir_entry = (tnode.type == VfsNodeType::Directory);
      }

      bool already_seen = false;
      for (uint32_t j = 0; j < seen_count; ++j) {
        if (strcmp(seen[j], component) == 0) {
          already_seen = true;
          break;
        }
      }
      if (already_seen) {
        continue;
      }
      if (seen_count < 32) {
        strncpy(seen[seen_count++], component, 31);
        seen[seen_count - 1][31] = '\0';
      }

      strncpy(entries[count].d_name, component, sizeof(entries[count].d_name) - 1);
      entries[count].d_name[sizeof(entries[count].d_name) - 1] = '\0';
      if (is_dir_entry) {
        entries[count].d_type = DT_DIR;
        entries[count].d_size = 0;
      } else {
        entries[count].d_type = tnode.type == VfsNodeType::CharDev ? DT_CHR : DT_REG;
        entries[count].d_size = static_cast<uint32_t>(tnode.size);
      }
      ++count;
    }
  } else {
    // Non-root: collect direct children under norm + "/".
    char prefix[kMaxPathLen];
    strncpy(prefix, norm, kMaxPathLen - 2);
    prefix[kMaxPathLen - 2] = '\0';
    const size_t prefix_len = strlen(prefix);
    prefix[prefix_len] = '/';
    prefix[prefix_len + 1] = '\0';
    const size_t full_prefix_len = prefix_len + 1;

    for (const auto& tnode : node_table) {
      if (count >= max_entries) {
        break;
      }
      if (tnode.name[0] == '\0') {
        continue;
      }
      const char* name = tnode.name;
      if (strncmp(name, prefix, full_prefix_len) != 0) {
        continue;
      }
      const char* child = name + full_prefix_len;
      if (child[0] == '\0' || strchr(child, '/') != nullptr) {
        continue;
      }

      strncpy(entries[count].d_name, child, sizeof(entries[count].d_name) - 1);
      entries[count].d_name[sizeof(entries[count].d_name) - 1] = '\0';
      if (tnode.type == VfsNodeType::Directory) {
        entries[count].d_type = DT_DIR;
        entries[count].d_size = 0;
      } else if (tnode.type == VfsNodeType::CharDev) {
        entries[count].d_type = DT_CHR;
        entries[count].d_size = static_cast<uint32_t>(tnode.size);
      } else {
        entries[count].d_type = DT_REG;
        entries[count].d_size = static_cast<uint32_t>(tnode.size);
      }
      ++count;
    }
  }

  return count;
}

void mount(const char* prefix, const FsOps* ops) {
  assert(prefix != nullptr && "mount(): null prefix");
  assert(ops != nullptr && "mount(): null fs ops");
  if (mount_count >= kMaxMounts) {
    return;
  }
  strncpy(mount_table[mount_count].prefix, prefix, kMaxPathLen - 1);
  mount_table[mount_count].prefix[kMaxPathLen - 1] = '\0';
  mount_table[mount_count].ops = ops;
  ++mount_count;
}

// Find the mount entry whose prefix is a prefix of abs_path.
static const FsOps* find_mount(const char* abs_path) {
  for (uint32_t i = 0; i < mount_count; ++i) {
    const size_t plen = strlen(mount_table[i].prefix);
    if (strncmp(abs_path, mount_table[i].prefix, plen) == 0 &&
        (abs_path[plen] == '/' || abs_path[plen] == '\0')) {
      return mount_table[i].ops;
    }
  }
  return nullptr;
}

int32_t fs_mkdir(const char* abs_path) {
  const FsOps* ops = find_mount(abs_path);
  if (ops == nullptr || ops->mkdir == nullptr) {
    return -ENOENT;
  }
  return ops->mkdir(abs_path);
}

int32_t fs_unlink(const char* abs_path) {
  const FsOps* ops = find_mount(abs_path);
  if (ops == nullptr || ops->unlink == nullptr) {
    return -ENOENT;
  }
  return ops->unlink(abs_path);
}

int32_t fs_rename(const char* old_path, const char* new_path) {
  const FsOps* ops = find_mount(old_path);
  if (ops == nullptr || ops->rename == nullptr) {
    return -ENOENT;
  }
  return ops->rename(old_path, new_path);
}

int32_t stat_path(const char* path, struct stat* buf) {
  assert(path != nullptr && "stat_path(): null path");
  assert(buf != nullptr && "stat_path(): null stat buffer");
  memset(buf, 0, sizeof(*buf));

  const VfsNode* node = lookup(path);
  if (node == nullptr) {
    if (is_directory(path)) {
      buf->st_mode = S_IFDIR | 0755;
      return 0;
    }
    return -ENOENT;
  }

  switch (node->type) {
    case VfsNodeType::File:
      buf->st_mode = S_IFREG | 0644;
      buf->st_size = static_cast<off_t>(node->size);
      break;
    case VfsNodeType::CharDev:
      buf->st_mode = S_IFCHR | 0666;
      break;
    case VfsNodeType::Directory:
      buf->st_mode = S_IFDIR | 0755;
      break;
  }

  // Use vector index as inode number.
  for (uint32_t i = 0; i < static_cast<uint32_t>(node_table.size()); ++i) {
    if (&node_table[i] == node) {
      buf->st_ino = static_cast<ino_t>(i);
      break;
    }
  }
  return 0;
}

void init_devfs() {
  assert(register_node("/dev/tty", VfsNodeType::CharDev, &tty_ops) != nullptr &&
         "init_devfs(): failed to register /dev/tty");
  assert(register_node("/dev/null", VfsNodeType::CharDev, &null_ops) != nullptr &&
         "init_devfs(): failed to register /dev/null");
  assert(register_node("/dev/kbd", VfsNodeType::CharDev, &kbd_ops) != nullptr &&
         "init_devfs(): failed to register /dev/kbd");

  if (Framebuffer::is_available()) {
    assert(register_node("/dev/fb", VfsNodeType::CharDev, &fb_ops) != nullptr &&
           "init_devfs(): failed to register /dev/fb");
  }
}

int32_t ioctl(FileDescription* fd, uint32_t request, void* arg) {
  auto* vfs_fd = fd->vfs;
  if ((vfs_fd == nullptr) || (vfs_fd->node == nullptr) || (vfs_fd->node->ops == nullptr) ||
      (vfs_fd->node->ops->ioctl == nullptr)) {
    return -25;  // -ENOTTY
  }
  return vfs_fd->node->ops->ioctl(vfs_fd->node, request, arg);
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
    strncpy(name, mod->name, kMaxPathLen - 1);
    name[kMaxPathLen - 1] = '\0';
    const bool is_elf = (name_len > 4 && strcmp(name + name_len - 4, ".elf") == 0);
    if (is_elf) {
      name[name_len - 4] = '\0';
    }

    // All modules go into /bin/<name>.
    char path[kMaxPathLen];
    strcpy(path, "/bin/");
    strncpy(path + 5, name, kMaxPathLen - 6);
    path[kMaxPathLen - 1] = '\0';

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
      strncpy(root_path + 1, name, kMaxPathLen - 2);
      root_path[kMaxPathLen - 1] = '\0';

      auto* root_node = register_node(root_path, VfsNodeType::File, &ramfs_ops);
      if (root_node != nullptr) {
        root_node->data = mod->data;
        root_node->size = mod->len;
      }
    }
  }
}

}  // namespace Vfs
