#pragma once

#include <optional.h>
#include <span.h>
#include <stdint.h>

// Maximum file descriptors per process.
static constexpr uint32_t kMaxFds = 16;

// Sentinel value returned by file_read / file_write when the caller should
// block and retry. syscall_dispatch detects this and restarts the syscall.
static constexpr int32_t kSyscallRestart = -0x7FFFFFFE;

// Forward declaration for pipe endpoints.
struct Pipe;

// Forward declaration for VFS open-file state.
struct VfsFileDescription;

// Types of open file descriptions.
enum class FileType : uint8_t {
  TerminalRead,   // stdin: reads from keyboard
  TerminalWrite,  // stdout/stderr: writes to VGA terminal
  PipeRead,       // read end of a pipe
  PipeWrite,      // write end of a pipe
  VfsNode,        // file/device opened through the VFS
};

// Reference-counted open file description.
//
// Multiple fds (across processes via fork, or via dup2 within a process)
// can point to the same FileDescription. When ref_count drops to zero
// the description is cleaned up via file_close().
struct FileDescription {
  FileType type;
  uint32_t ref_count;
  Pipe* pipe;                // non-null for PipeRead / PipeWrite only
  VfsFileDescription* vfs;   // non-null for VfsNode only

  void ref() { ++ref_count; }
};

// Read up to buf.size() bytes. Returns bytes read, 0 for EOF, or kSyscallRestart
// to block. Returns -1 on error (e.g. not a readable fd).
[[nodiscard]] int32_t file_read(FileDescription* fd, std::span<uint8_t> buf);

// Write up to buf.size() bytes. Returns bytes written, kSyscallRestart to block,
// or -1 on error.
[[nodiscard]] int32_t file_write(FileDescription* fd, std::span<const uint8_t> buf);

// Decrement ref_count and perform type-specific cleanup when it reaches 0.
// Terminal descriptions are static singletons and are never freed.
void file_close(FileDescription* fd);

// Initialise fds[0..2] to stdin/stdout/stderr (terminal descriptions).
// Remaining slots are set to nullptr.
void fd_init_stdio(std::span<FileDescription*> fds);

// Find the lowest free fd slot. Returns the index, or nullopt if full.
[[nodiscard]] std::optional<uint32_t> fd_alloc(std::span<FileDescription*> fds);

// Find the lowest free fd slot >= min_fd. Returns the index, or nullopt if full.
[[nodiscard]] std::optional<uint32_t> fd_alloc_from(std::span<FileDescription*> fds,
                                                     uint32_t min_fd);
