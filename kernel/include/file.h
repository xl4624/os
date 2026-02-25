#pragma once

#include <stdint.h>

// Maximum file descriptors per process.
static constexpr uint32_t kMaxFds = 16;

// Sentinel value returned by file_read / file_write when the caller should
// block and retry. syscall_dispatch detects this and restarts the syscall.
static constexpr int32_t kSyscallRestart = -0x7FFFFFFE;

// Forward declaration for pipe endpoints.
struct Pipe;

// Types of open file descriptions.
enum class FileType : uint8_t {
  TerminalRead,   // stdin: reads from keyboard
  TerminalWrite,  // stdout/stderr: writes to VGA terminal
  PipeRead,       // read end of a pipe
  PipeWrite,      // write end of a pipe
};

// Reference-counted open file description.
//
// Multiple fds (across processes via fork, or via dup2 within a process)
// can point to the same FileDescription. When ref_count drops to zero
// the description is cleaned up via file_close().
struct FileDescription {
  FileType type;
  uint32_t ref_count;
  Pipe* pipe;  // non-null for PipeRead / PipeWrite only

  void ref() { ++ref_count; }
};

// Read up to count bytes. Returns bytes read, 0 for EOF, or kSyscallRestart
// to block. Returns -1 on error (e.g. not a readable fd).
int32_t file_read(FileDescription* fd, uint8_t* buf, uint32_t count);

// Write up to count bytes. Returns bytes written, kSyscallRestart to block,
// or -1 on error.
int32_t file_write(FileDescription* fd, const uint8_t* buf, uint32_t count);

// Decrement ref_count and perform type-specific cleanup when it reaches 0.
// Terminal descriptions are static singletons and are never freed.
void file_close(FileDescription* fd);

// Initialise fds[0..2] to stdin/stdout/stderr (terminal descriptions).
// Remaining slots are set to nullptr.
void fd_init_stdio(FileDescription* fds[]);

// Find the lowest free fd slot in fds[]. Returns index or -1 if full.
int32_t fd_alloc(FileDescription* fds[]);

// Find the lowest free fd slot >= min_fd. Returns index or -1 if full.
int32_t fd_alloc_from(FileDescription* fds[], uint32_t min_fd);
