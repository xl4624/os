#include "vfs.h"

#include <algorithm.h>
#include <errno.h>
#include <string.h>
#include <sys/fb.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "framebuffer.h"
#include "keyboard.h"
#include "modules.h"
#include "scheduler.h"
#include "tty.h"

namespace {

uint32_t node_count = 0;
VfsNode node_table[kMaxVfsNodes];

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

const VfsOps tty_ops = {.read = tty_read, .write = tty_write, .ioctl = tty_ioctl};

int32_t null_read([[maybe_unused]] VfsNode* node, [[maybe_unused]] std::span<uint8_t> buf,
                  [[maybe_unused]] uint32_t offset) {
  return 0;  // EOF
}

int32_t null_write([[maybe_unused]] VfsNode* node, std::span<const uint8_t> buf,
                   [[maybe_unused]] uint32_t offset) {
  return static_cast<int32_t>(buf.size());  // discard
}

const VfsOps null_ops = {.read = null_read, .write = null_write, .ioctl = nullptr};

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

const VfsOps kbd_ops = {.read = kbd_read, .write = kbd_write, .ioctl = nullptr};

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

const VfsOps fb_ops = {.read = fb_read, .write = fb_write, .ioctl = nullptr};

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

const VfsOps ramfs_ops = {.read = ramfs_read, .write = ramfs_write, .ioctl = nullptr};

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
  strncpy(node->name, path, kMaxPathLen - 1);
  node->name[kMaxPathLen - 1] = '\0';
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

bool is_directory(const char* path) {
  if (path == nullptr || path[0] != '/') {
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
  // Build the prefix that a child node would start with.
  char prefix[kMaxPathLen];
  strncpy(prefix, norm, kMaxPathLen - 2);
  prefix[kMaxPathLen - 2] = '\0';
  const size_t prefix_len = strlen(prefix);
  prefix[prefix_len] = '/';
  prefix[prefix_len + 1] = '\0';

  for (uint32_t i = 0; i < node_count; ++i) {
    if (strncmp(node_table[i].name, prefix, prefix_len + 1) == 0) {
      return true;
    }
  }
  return false;
}

uint32_t getdents(const char* path, dirent* entries, uint32_t max_entries) {
  if (path == nullptr || path[0] != '/' || entries == nullptr || max_entries == 0) {
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
    // Use a small fixed array to deduplicate virtual directory names.
    char seen[32][32];
    uint32_t seen_count = 0;

    for (uint32_t i = 0; i < node_count && count < max_entries; ++i) {
      const char* name = node_table[i].name;
      if (name[0] != '/') {
        continue;
      }

      const char* start = name + 1;
      const char* slash = strchr(start, '/');

      char component[128];
      bool is_virtual_dir;
      if (slash != nullptr) {
        // e.g. "/bin/sh" -> component "bin", virtual dir
        const auto clen = static_cast<size_t>(slash - start);
        if (clen == 0 || clen >= sizeof(component)) {
          continue;
        }
        strncpy(component, start, clen);
        component[clen] = '\0';
        is_virtual_dir = true;
      } else {
        // e.g. "/doom1.wad" -> direct root file
        strncpy(component, start, sizeof(component) - 1);
        component[sizeof(component) - 1] = '\0';
        is_virtual_dir = false;
      }

      // Deduplicate virtual dirs via the seen[] list.
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
      if (is_virtual_dir) {
        entries[count].d_type = DT_DIR;
        entries[count].d_size = 0;
      } else {
        entries[count].d_type = node_table[i].type == VfsNodeType::CharDev ? DT_CHR : DT_REG;
        entries[count].d_size = static_cast<uint32_t>(node_table[i].size);
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

    for (uint32_t i = 0; i < node_count && count < max_entries; ++i) {
      const char* name = node_table[i].name;
      if (strncmp(name, prefix, full_prefix_len) != 0) {
        continue;
      }
      const char* child = name + full_prefix_len;
      if (child[0] == '\0' || strchr(child, '/') != nullptr) {
        continue;  // skip empty or nested paths
      }

      strncpy(entries[count].d_name, child, sizeof(entries[count].d_name) - 1);
      entries[count].d_name[sizeof(entries[count].d_name) - 1] = '\0';
      entries[count].d_type = node_table[i].type == VfsNodeType::CharDev ? DT_CHR : DT_REG;
      entries[count].d_size = static_cast<uint32_t>(node_table[i].size);
      ++count;
    }
  }

  return count;
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
