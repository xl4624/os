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

#include "framebuffer.h"
#include "keyboard.h"
#include "terminal.h"
#include "modules.h"
#include "scheduler.h"
#include "tty.h"

namespace {

VfsNode* root_node = nullptr;

// ===========================================================================
// Tree-based mount support
// ===========================================================================

// Find the nearest ancestor (or self) with mount_ops set.
const FsOps* find_mount_ops(const VfsNode* node) {
  for (const VfsNode* n = node; n != nullptr; n = n->parent) {
    if (n->mount_ops != nullptr) {
      return n->mount_ops;
    }
  }
  return nullptr;
}

// ===========================================================================
// Tree helpers
// ===========================================================================

// Find a direct child of parent with the given name.
VfsNode* find_child(VfsNode* parent, const char* name) {
  for (VfsNode* c = parent->first_child; c != nullptr; c = c->next_sibling) {
    if (strcmp(c->name, name) == 0) {
      return c;
    }
  }
  return nullptr;
}

// Add a child to a parent's child list (prepend).
void add_child(VfsNode* parent, VfsNode* child) {
  child->parent = parent;
  child->next_sibling = parent->first_child;
  parent->first_child = child;
}

// Remove a child from its parent's child list.
void remove_child(VfsNode* child) {
  VfsNode* parent = child->parent;
  if (parent == nullptr) {
    return;
  }
  if (parent->first_child == child) {
    parent->first_child = child->next_sibling;
  } else {
    for (VfsNode* c = parent->first_child; c != nullptr; c = c->next_sibling) {
      if (c->next_sibling == child) {
        c->next_sibling = child->next_sibling;
        break;
      }
    }
  }
  child->parent = nullptr;
  child->next_sibling = nullptr;
}

// Allocate and initialise a new tree node.
// Default mode based on node type.
mode_t default_mode(VfsNodeType type) {
  switch (type) {
    case VfsNodeType::File:
      return S_IFREG | 0644;
    case VfsNodeType::CharDev:
      return S_IFCHR | 0666;
    case VfsNodeType::Directory:
      return S_IFDIR | 0755;
  }
  return 0;
}

VfsNode* make_node(const char* name, VfsNodeType type, const VfsOps* ops, mode_t mode = 0) {
  auto* node = new VfsNode{};
  strncpy(node->name, name, kMaxNameLen - 1);
  node->name[kMaxNameLen - 1] = '\0';
  node->type = type;
  node->ops = ops;
  node->data = nullptr;
  node->size = 0;
  node->priv = nullptr;
  node->mode = (mode != 0) ? mode : default_mode(type);
  node->uid = 0;
  node->gid = 0;
  node->parent = nullptr;
  node->first_child = nullptr;
  node->next_sibling = nullptr;
  node->mount_ops = nullptr;
  return node;
}

// Walk the tree from root following the given absolute path.
// Returns the node at path, or nullptr if any component is missing.
// TODO: tree_lookup and register_node share the same component-by-component
// path walk. Extract a shared next_component() iterator to deduplicate.
VfsNode* tree_lookup(const char* path) {
  if (root_node == nullptr) {
    return nullptr;
  }
  if (path[0] == '/' && path[1] == '\0') {
    return root_node;
  }

  VfsNode* cur = root_node;
  const char* p = path + 1;  // skip leading '/'

  while (*p != '\0') {
    const char* slash = strchr(p, '/');
    char component[kMaxNameLen];
    size_t clen;
    if (slash != nullptr) {
      clen = static_cast<size_t>(slash - p);
    } else {
      clen = strlen(p);
    }
    if (clen == 0) {
      // double slash or trailing slash -- skip
      ++p;
      continue;
    }
    if (clen >= kMaxNameLen) {
      return nullptr;
    }
    memcpy(component, p, clen);
    component[clen] = '\0';

    cur = find_child(cur, component);
    if (cur == nullptr) {
      return nullptr;
    }

    p += clen;
    if (*p == '/') {
      ++p;
    }
  }
  return cur;
}

// Build the full absolute path of a node into buf. Returns the length written.
[[maybe_unused]] uint32_t node_path(const VfsNode* node, char* buf, uint32_t buf_size) {
  if (node == root_node || node->parent == nullptr) {
    if (buf_size > 1) {
      buf[0] = '/';
      buf[1] = '\0';
      return 1;
    }
    buf[0] = '\0';
    return 0;
  }

  // Build path by walking up to root, then reversing.
  const VfsNode* ancestors[32];
  uint32_t depth = 0;
  for (const VfsNode* n = node; n != nullptr && n != root_node && depth < 32; n = n->parent) {
    ancestors[depth++] = n;
  }

  uint32_t pos = 0;
  for (uint32_t i = depth; i > 0; --i) {
    if (pos + 1 < buf_size) {
      buf[pos++] = '/';
    }
    const char* name = ancestors[i - 1]->name;
    const size_t nlen = strlen(name);
    for (size_t j = 0; j < nlen && pos + 1 < buf_size; ++j) {
      buf[pos++] = name[j];
    }
  }
  buf[pos] = '\0';
  return pos;
}

// Recursively delete a tree node and all its children.
// TODO: make iterative to avoid stack overflow on deep trees.
// TODO: free node->priv (FAT metadata) before deleting -- currently leaks on unmount.
void delete_tree(VfsNode* node) {
  if (node == nullptr) {
    return;
  }
  VfsNode* child = node->first_child;
  while (child != nullptr) {
    VfsNode* next = child->next_sibling;
    delete_tree(child);
    child = next;
  }
  delete node;
}

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
      ws->ws_row = static_cast<uint16_t>(kTerminal.rows());
      ws->ws_col = static_cast<uint16_t>(kTerminal.cols());
      ws->ws_xpixel = static_cast<uint16_t>(Framebuffer::info().width);
      ws->ws_ypixel = static_cast<uint16_t>(Framebuffer::info().height);
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
  delete_tree(root_node);
  root_node = make_node("", VfsNodeType::Directory, nullptr);
}

VfsNode* register_node(const char* path, VfsNodeType type, const VfsOps* ops, mode_t mode) {
  assert(path != nullptr && path[0] == '/' &&
         "VfsNode::register_node: path must be non-null and start with '/'");
  if (strlen(path) >= kMaxPathLen) {
    return nullptr;
  }

  // Walk the tree, creating intermediate Directory nodes as needed.
  VfsNode* cur = root_node;
  const char* p = path + 1;  // skip leading '/'

  while (*p != '\0') {
    const char* slash = strchr(p, '/');
    char component[kMaxNameLen];
    size_t clen;
    if (slash != nullptr) {
      clen = static_cast<size_t>(slash - p);
    } else {
      clen = strlen(p);
    }
    if (clen == 0) {
      ++p;
      continue;
    }
    if (clen >= kMaxNameLen) {
      return nullptr;
    }
    memcpy(component, p, clen);
    component[clen] = '\0';

    p += clen;
    if (*p == '/') {
      ++p;
    }

    if (*p == '\0') {
      // This is the final component -- create the leaf node.
      VfsNode* existing = find_child(cur, component);
      if (existing != nullptr) {
        // Node already exists; update type and ops.
        existing->type = type;
        existing->ops = ops;
        if (mode != 0) {
          existing->mode = mode;
        }
        return existing;
      }
      VfsNode* leaf = make_node(component, type, ops, mode);
      add_child(cur, leaf);
      return leaf;
    }

    // Intermediate component -- find or create directory.
    VfsNode* child = find_child(cur, component);
    if (child == nullptr) {
      child = make_node(component, VfsNodeType::Directory, nullptr);
      add_child(cur, child);
    }
    cur = child;
  }

  // Path was just "/" -- return root.
  return root_node;
}

void unregister_node(const char* path) {
  assert(path != nullptr && "unregister_node(): null path");
  VfsNode* node = tree_lookup(path);
  if (node != nullptr && node != root_node) {
    remove_child(node);
    delete_tree(node);
  }
}

VfsNode* lookup(const char* path) {
  assert(path != nullptr && path[0] == '/' && "lookup(): path must be non-null and absolute");
  return tree_lookup(path);
}

int32_t open(const char* path, int32_t flags, mode_t mode) {
  assert(path != nullptr && path[0] == '/' && "open(): path must be non-null and absolute");

  VfsNode* node = lookup(path);

  // O_CREAT: try to create the file via the mounted filesystem.
  // TODO: the parent-path extraction (strrchr + memcpy) is duplicated in
  // open(), fs_mkdir(), fat_mkdir(), and fat_create(). Extract a path_parent() helper.
  if (node == nullptr && (flags & O_CREAT) != 0) {
    // Find the parent directory to determine which mount to use.
    const char* last_slash = strrchr(path, '/');
    if (last_slash != nullptr) {
      char parent_path[kMaxPathLen];
      const auto parent_len = static_cast<size_t>(last_slash - path);
      if (parent_len == 0) {
        parent_path[0] = '/';
        parent_path[1] = '\0';
      } else if (parent_len < kMaxPathLen) {
        memcpy(parent_path, path, parent_len);
        parent_path[parent_len] = '\0';
      } else {
        parent_path[0] = '\0';
      }
      const VfsNode* parent_node = tree_lookup(parent_path);
      if (parent_node != nullptr) {
        const FsOps* fs = find_mount_ops(parent_node);
        if (fs != nullptr && fs->create != nullptr) {
          node = fs->create(path, flags, mode);
        }
      }
    }
  }

  if (node == nullptr) {
    return -ENOENT;
  }

  Process* proc = Scheduler::current();
  if (proc == nullptr) {
    return -ESRCH;
  }

  // Permission check (root bypasses).
  // TODO: use S_IRUSR/S_IWUSR/O_ACCMODE constants instead of magic numbers.
  if (proc->uid != 0) {
    const int access = flags & 3;
    mode_t need = 0;
    if (access == O_RDONLY || access == O_RDWR) {
      need |= 4;
    }
    if (access == O_WRONLY || access == O_RDWR) {
      need |= 2;
    }
    mode_t have;
    if (proc->uid == node->uid) {
      have = (node->mode >> 6) & 7;
    } else if (proc->gid == node->gid) {
      have = (node->mode >> 3) & 7;
    } else {
      have = node->mode & 7;
    }
    if ((have & need) != need) {
      return -EACCES;
    }
  }

  // O_TRUNC: truncate an existing writable file.
  if ((flags & O_TRUNC) != 0 && node->type == VfsNodeType::File && node->ops != nullptr &&
      node->ops->truncate != nullptr) {
    node->ops->truncate(node);
  }

  auto slot = fd_alloc(proc->fds);
  if (!slot) {
    return -EMFILE;
  }

  auto* vfs_fd = new VfsFileDescription{.node = node, .offset = 0, .open_flags = flags};
  if (vfs_fd == nullptr) {
    return -ENOMEM;
  }

  auto* desc = new FileDescription{
      .type = FileType::VfsNode, .ref_count = 1, .pipe = nullptr, .vfs = vfs_fd};
  if (desc == nullptr) {
    delete vfs_fd;
    return -ENOMEM;
  }

  proc->fds[*slot] = desc;
  proc->fd_flags[*slot] = ((flags & O_CLOEXEC) != 0) ? FD_CLOEXEC : 0;
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
  // Strip trailing slash for lookup.
  char norm[kMaxPathLen];
  strncpy(norm, path, kMaxPathLen - 1);
  norm[kMaxPathLen - 1] = '\0';
  const size_t len = strlen(norm);
  if (len > 1 && norm[len - 1] == '/') {
    norm[len - 1] = '\0';
  }

  const VfsNode* node = tree_lookup(norm);
  if (node == nullptr) {
    return false;
  }
  return node->type == VfsNodeType::Directory;
}

uint32_t getdents(const char* path, dirent* entries, uint32_t max_entries) {
  assert(path != nullptr && path[0] == '/' && "getdents(): path must be non-null and absolute");
  assert(entries != nullptr && "getdents(): null entries buffer");
  if (max_entries == 0) {
    return 0;
  }

  // Strip trailing slash.
  char norm[kMaxPathLen];
  strncpy(norm, path, kMaxPathLen - 1);
  norm[kMaxPathLen - 1] = '\0';
  const size_t norm_len_raw = strlen(norm);
  if (norm_len_raw > 1 && norm[norm_len_raw - 1] == '/') {
    norm[norm_len_raw - 1] = '\0';
  }

  const VfsNode* dir = tree_lookup(norm);
  if (dir == nullptr) {
    return 0;
  }

  uint32_t count = 0;
  for (VfsNode* child = dir->first_child; child != nullptr && count < max_entries;
       child = child->next_sibling) {
    strncpy(entries[count].d_name, child->name, sizeof(entries[count].d_name) - 1);
    entries[count].d_name[sizeof(entries[count].d_name) - 1] = '\0';
    if (child->type == VfsNodeType::Directory) {
      entries[count].d_type = DT_DIR;
      entries[count].d_size = 0;
    } else if (child->type == VfsNodeType::CharDev) {
      entries[count].d_type = DT_CHR;
      entries[count].d_size = static_cast<uint32_t>(child->size);
    } else {
      entries[count].d_type = DT_REG;
      entries[count].d_size = static_cast<uint32_t>(child->size);
    }
    ++count;
  }

  return count;
}

int32_t mount(const char* path, const FsOps* ops) {
  assert(path != nullptr && "mount(): null path");
  assert(ops != nullptr && "mount(): null fs ops");

  // Find or create the directory node at the mount point.
  VfsNode* node = tree_lookup(path);
  if (node == nullptr) {
    node = register_node(path, VfsNodeType::Directory, nullptr);
    if (node == nullptr) {
      return -ENOMEM;
    }
  }

  node->mount_ops = ops;

  // Call the filesystem's mount callback if provided.
  if (ops->mount != nullptr) {
    const int32_t rc = ops->mount(node);
    if (rc < 0) {
      node->mount_ops = nullptr;
      return rc;
    }
  }

  return 0;
}

int32_t unmount(const char* path) {
  assert(path != nullptr && "unmount(): null path");

  VfsNode* node = tree_lookup(path);
  if (node == nullptr || node->mount_ops == nullptr) {
    return -EINVAL;
  }

  // Call the filesystem's unmount callback if provided.
  if (node->mount_ops->unmount != nullptr) {
    node->mount_ops->unmount(node);
  }

  // Remove all children (the mounted filesystem's subtree).
  while (node->first_child != nullptr) {
    VfsNode* child = node->first_child;
    remove_child(child);
    delete_tree(child);
  }

  node->mount_ops = nullptr;
  return 0;
}

int32_t fs_mkdir(const char* abs_path) {
  // Walk the tree to find the nearest mount ancestor.
  // First check if parent exists.
  const VfsNode* node = tree_lookup(abs_path);
  const VfsNode* start = (node != nullptr) ? node : root_node;
  // If the path doesn't exist, find the parent.
  if (node == nullptr) {
    const char* last_slash = strrchr(abs_path, '/');
    if (last_slash != nullptr) {
      char parent_path[kMaxPathLen];
      const auto plen = static_cast<size_t>(last_slash - abs_path);
      if (plen == 0) {
        parent_path[0] = '/';
        parent_path[1] = '\0';
      } else {
        memcpy(parent_path, abs_path, plen);
        parent_path[plen] = '\0';
      }
      const VfsNode* parent = tree_lookup(parent_path);
      if (parent != nullptr) {
        start = parent;
      }
    }
  }
  const FsOps* ops = find_mount_ops(start);
  if (ops == nullptr || ops->mkdir == nullptr) {
    return -ENOENT;
  }
  return ops->mkdir(abs_path);
}

int32_t fs_unlink(const char* abs_path) {
  const VfsNode* node = tree_lookup(abs_path);
  const FsOps* ops = find_mount_ops(node != nullptr ? node : root_node);
  if (ops == nullptr || ops->unlink == nullptr) {
    return -ENOENT;
  }
  return ops->unlink(abs_path);
}

int32_t fs_rename(const char* old_path, const char* new_path) {
  const VfsNode* node = tree_lookup(old_path);
  const FsOps* ops = find_mount_ops(node != nullptr ? node : root_node);
  if (ops == nullptr || ops->rename == nullptr) {
    return -ENOENT;
  }
  return ops->rename(old_path, new_path);
}

int32_t stat_node(const VfsNode* node, struct stat* buf) {
  assert(node != nullptr && "stat_node(): null node");
  assert(buf != nullptr && "stat_node(): null stat buffer");
  memset(buf, 0, sizeof(*buf));

  buf->st_mode = node->mode;
  buf->st_uid = static_cast<uid_t>(node->uid);
  buf->st_gid = static_cast<gid_t>(node->gid);
  if (node->type == VfsNodeType::File) {
    buf->st_size = static_cast<off_t>(node->size);
  }

  // Use node address as inode number.
  buf->st_ino = static_cast<ino_t>(reinterpret_cast<uintptr_t>(node));
  return 0;
}

int32_t stat_path(const char* path, struct stat* buf) {
  assert(path != nullptr && "stat_path(): null path");
  const VfsNode* node = lookup(path);
  if (node == nullptr) {
    return -ENOENT;
  }
  return stat_node(node, buf);
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
    return -ENOTTY;
  }
  return vfs_fd->node->ops->ioctl(vfs_fd->node, request, arg);
}

int32_t tty_ioctl(uint32_t request, void* arg) {
  return ::tty_ioctl(nullptr, request, arg);
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

      auto* root_file = register_node(root_path, VfsNodeType::File, &ramfs_ops);
      if (root_file != nullptr) {
        root_file->data = mod->data;
        root_file->size = mod->len;
      }
    }
  }
}

}  // namespace Vfs
