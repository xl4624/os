#include "pipe.h"

#include "file.h"
#include "heap.h"

int32_t pipe_read(Pipe* pipe, uint8_t* buf, uint32_t count) {
  uint32_t bytes_read = 0;
  for (uint32_t i = 0; i < count; ++i) {
    char c;
    if (!pipe->buffer.pop(c)) {
      break;
    }
    buf[i] = static_cast<uint8_t>(c);
    ++bytes_read;
  }

  if (bytes_read > 0) {
    return static_cast<int32_t>(bytes_read);
  }

  // Buffer empty. If writers still exist, block the caller.
  if (pipe->writers > 0) {
    return kSyscallRestart;
  }

  // No writers remaining: EOF.
  return 0;
}

int32_t pipe_write(Pipe* pipe, const uint8_t* buf, uint32_t count) {
  // No readers: broken pipe.
  if (pipe->readers == 0) {
    return -1;
  }

  uint32_t bytes_written = 0;
  for (uint32_t i = 0; i < count; ++i) {
    if (!pipe->buffer.push(static_cast<char>(buf[i]))) {
      break;
    }
    ++bytes_written;
  }

  if (bytes_written > 0) {
    return static_cast<int32_t>(bytes_written);
  }

  // Buffer full and readers exist: block the caller.
  return kSyscallRestart;
}

static void pipe_maybe_free(Pipe* pipe) {
  if (pipe->readers == 0 && pipe->writers == 0) {
    kfree(pipe);
  }
}

void pipe_close_read(Pipe* pipe) {
  if (!pipe) {
    return;
  }
  if (pipe->readers > 0) {
    --pipe->readers;
  }
  pipe_maybe_free(pipe);
}

void pipe_close_write(Pipe* pipe) {
  if (!pipe) {
    return;
  }
  if (pipe->writers > 0) {
    --pipe->writers;
  }
  pipe_maybe_free(pipe);
}
