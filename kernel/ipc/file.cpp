#include "file.h"

#include <string.h>

#include "heap.h"
#include "keyboard.h"
#include "pipe.h"
#include "tty.h"

// ---------------------------------------------------------------------------
// Terminal singletons
// ---------------------------------------------------------------------------

static FileDescription g_terminal_read = {FileType::TerminalRead, 1, nullptr};
static FileDescription g_terminal_write = {FileType::TerminalWrite, 1, nullptr};

// ---------------------------------------------------------------------------
// File operations dispatch
// ---------------------------------------------------------------------------

int32_t file_read(FileDescription* fd, uint8_t* buf, uint32_t count) {
  switch (fd->type) {
    case FileType::TerminalRead: {
      char* cbuf = reinterpret_cast<char*>(buf);
      size_t n = kKeyboard.read(cbuf, count);
      return static_cast<int32_t>(n);
    }
    case FileType::PipeRead:
      return pipe_read(fd->pipe, buf, count);
    default:
      return -1;
  }
}

int32_t file_write(FileDescription* fd, const uint8_t* buf, uint32_t count) {
  switch (fd->type) {
    case FileType::TerminalWrite: {
      const char* data = reinterpret_cast<const char*>(buf);
      for (uint32_t i = 0; i < count; ++i) {
        terminal_putchar(data[i]);
      }
      return static_cast<int32_t>(count);
    }
    case FileType::PipeWrite:
      return pipe_write(fd->pipe, buf, count);
    default:
      return -1;
  }
}

void file_close(FileDescription* fd) {
  // Terminal singletons are never freed; just decrement their ref count.
  if (fd->type == FileType::TerminalRead || fd->type == FileType::TerminalWrite) {
    if (fd->ref_count > 0) {
      --fd->ref_count;
    }
    return;
  }

  if (fd->ref_count > 1) {
    --fd->ref_count;
    return;
  }

  switch (fd->type) {
    case FileType::PipeRead:
      pipe_close_read(fd->pipe);
      kfree(fd);
      return;
    case FileType::PipeWrite:
      pipe_close_write(fd->pipe);
      kfree(fd);
      return;
    default:
      return;
  }
}

// ---------------------------------------------------------------------------
// FD table helpers
// ---------------------------------------------------------------------------

void fd_init_stdio(FileDescription* fds[]) {
  fds[0] = &g_terminal_read;
  fds[1] = &g_terminal_write;
  fds[2] = &g_terminal_write;

  // Increment ref counts for the new references (fd 0 adds 1 to read,
  // fds 1 and 2 each add 1 to write).
  ++g_terminal_read.ref_count;
  g_terminal_write.ref_count += 2;

  for (uint32_t i = 3; i < kMaxFds; ++i) {
    fds[i] = nullptr;
  }
}

int32_t fd_alloc(FileDescription* fds[]) { return fd_alloc_from(fds, 0); }

int32_t fd_alloc_from(FileDescription* fds[], uint32_t min_fd) {
  for (uint32_t i = min_fd; i < kMaxFds; ++i) {
    if (!fds[i]) {
      return static_cast<int32_t>(i);
    }
  }
  return -1;
}
