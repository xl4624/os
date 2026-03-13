#include <span.h>
#include <string.h>
#include <sys/syscall.h>
#include <unique_ptr.h>

#include "file.h"
#include "ktest.h"
#include "pipe.h"
#include "process.h"
#include "scheduler.h"
#include "syscall.h"

namespace {

// Helper: creates a pipe directly via kernel APIs (bypassing sys_pipe which
// would reject kernel-space pointers during ktest).
bool make_pipe(uint32_t& rfd, uint32_t& wfd) {
  Process* proc = Scheduler::current();

  auto rfd_opt = fd_alloc(proc->fds);
  if (!rfd_opt) {
    return false;
  }
  rfd = *rfd_opt;

  auto wfd_opt = fd_alloc_from(proc->fds, rfd + 1);
  if (!wfd_opt) {
    return false;
  }
  wfd = *wfd_opt;

  auto pipe = std::make_unique<Pipe>();
  if (!pipe) {
    return false;
  }

  auto rd = std::make_unique<FileDescription>(
      FileDescription{FileType::PipeRead, 1, pipe.get(), nullptr});
  auto wr = std::make_unique<FileDescription>(
      FileDescription{FileType::PipeWrite, 1, pipe.get(), nullptr});
  if (!rd || !wr) {
    return false;
  }

  ++pipe->readers;
  ++pipe->writers;

  proc->fds[rfd] = rd.release();
  proc->fds[wfd] = wr.release();
  (void)pipe.release();
  return true;
}

void close_fd(uint32_t fd_num) {
  Process* proc = Scheduler::current();
  if (fd_num < kMaxFds && proc->fds[fd_num]) {
    file_close(proc->fds[fd_num]);
    proc->fds[fd_num] = nullptr;
  }
}

}  // namespace

// ===========================================================================
// SYS_PIPE
// ===========================================================================

TEST(pipe, create_returns_two_fds) {
  uint32_t rfd, wfd;
  ASSERT_TRUE(make_pipe(rfd, wfd));
  ASSERT(rfd != wfd);

  Process* proc = Scheduler::current();
  ASSERT_NOT_NULL(proc->fds[rfd]);
  ASSERT_NOT_NULL(proc->fds[wfd]);
  ASSERT_EQ(proc->fds[rfd]->type, FileType::PipeRead);
  ASSERT_EQ(proc->fds[wfd]->type, FileType::PipeWrite);

  close_fd(rfd);
  close_fd(wfd);
}

// ===========================================================================
// Pipe read / write
// ===========================================================================

TEST(pipe, write_then_read) {
  uint32_t rfd, wfd;
  ASSERT_TRUE(make_pipe(rfd, wfd));

  Process* proc = Scheduler::current();
  Pipe* pipe = proc->fds[wfd]->pipe;

  // Write directly to the pipe.
  const uint8_t msg[] = {'h', 'e', 'l', 'l', 'o'};
  int32_t written = pipe_write(pipe, msg);
  ASSERT_EQ(written, 5);

  // Read directly from the pipe.
  uint8_t buf[8] = {};
  int32_t nread = pipe_read(pipe, buf);
  ASSERT_EQ(nread, 5);
  ASSERT(memcmp(buf, "hello", 5) == 0);

  close_fd(rfd);
  close_fd(wfd);
}

TEST(pipe, read_empty_with_writers_returns_restart) {
  uint32_t rfd, wfd;
  ASSERT_TRUE(make_pipe(rfd, wfd));

  Process* proc = Scheduler::current();
  Pipe* pipe = proc->fds[rfd]->pipe;
  ASSERT(pipe->buffer.is_empty());
  ASSERT(pipe->writers > 0);

  // Directly call pipe_read to check the return value (syscall_dispatch
  // would turn kSyscallRestart into an EIP rewind).
  uint8_t buf[4];
  int32_t ret = pipe_read(pipe, buf);
  ASSERT_EQ(ret, kSyscallRestart);

  close_fd(rfd);
  close_fd(wfd);
}

TEST(pipe, read_eof_no_writers) {
  uint32_t rfd, wfd;
  ASSERT_TRUE(make_pipe(rfd, wfd));

  // Close the write end so there are no writers.
  close_fd(wfd);

  Process* proc = Scheduler::current();
  Pipe* pipe = proc->fds[rfd]->pipe;
  ASSERT_EQ(pipe->writers, 0u);

  uint8_t buf[4];
  int32_t ret = pipe_read(pipe, buf);
  ASSERT_EQ(ret, 0);

  close_fd(rfd);
}

TEST(pipe, write_no_readers_returns_epipe) {
  uint32_t rfd, wfd;
  ASSERT_TRUE(make_pipe(rfd, wfd));

  // Close the read end so there are no readers.
  close_fd(rfd);

  Process* proc = Scheduler::current();
  Pipe* pipe = proc->fds[wfd]->pipe;
  ASSERT_EQ(pipe->readers, 0u);

  const uint8_t data[] = {1, 2, 3};
  int32_t ret = pipe_write(pipe, data);
  ASSERT_EQ(ret, -1);

  close_fd(wfd);
}

TEST(pipe, buffer_full_returns_restart) {
  uint32_t rfd, wfd;
  ASSERT_TRUE(make_pipe(rfd, wfd));

  Process* proc = Scheduler::current();
  Pipe* pipe = proc->fds[wfd]->pipe;

  // Fill the buffer.
  uint8_t fill[kPipeBufferSize];
  memset(fill, 'A', sizeof(fill));
  int32_t ret = pipe_write(pipe, fill);
  ASSERT_EQ(ret, static_cast<int32_t>(kPipeBufferSize));
  ASSERT_TRUE(pipe->buffer.is_full());

  // Another write should block.
  uint8_t extra = 'B';
  ret = pipe_write(pipe, std::span<const uint8_t>(&extra, 1));
  ASSERT_EQ(ret, kSyscallRestart);

  close_fd(rfd);
  close_fd(wfd);
}

// ===========================================================================
// Pipe close semantics
// ===========================================================================

TEST(pipe, close_read_decrements_readers) {
  uint32_t rfd, wfd;
  ASSERT_TRUE(make_pipe(rfd, wfd));

  Process* proc = Scheduler::current();
  Pipe* pipe = proc->fds[rfd]->pipe;
  ASSERT_EQ(pipe->readers, 1u);

  close_fd(rfd);
  ASSERT_EQ(pipe->readers, 0u);

  close_fd(wfd);
}

TEST(pipe, close_write_decrements_writers) {
  uint32_t rfd, wfd;
  ASSERT_TRUE(make_pipe(rfd, wfd));

  Process* proc = Scheduler::current();
  Pipe* pipe = proc->fds[wfd]->pipe;
  ASSERT_EQ(pipe->writers, 1u);

  close_fd(wfd);
  ASSERT_EQ(pipe->writers, 0u);

  close_fd(rfd);
}

TEST(pipe, partial_read) {
  uint32_t rfd, wfd;
  ASSERT_TRUE(make_pipe(rfd, wfd));

  Process* proc = Scheduler::current();
  Pipe* pipe = proc->fds[wfd]->pipe;

  // Write 5 bytes, read 3, then read the remaining 2.
  const uint8_t msg[] = {10, 20, 30, 40, 50};
  ASSERT_NE(pipe_write(pipe, msg), -1);

  uint8_t buf[3];
  int32_t ret = pipe_read(pipe, buf);
  ASSERT_EQ(ret, 3);
  ASSERT_EQ(buf[0], 10);
  ASSERT_EQ(buf[1], 20);
  ASSERT_EQ(buf[2], 30);

  uint8_t buf2[4];
  ret = pipe_read(pipe, buf2);
  ASSERT_EQ(ret, 2);
  ASSERT_EQ(buf2[0], 40);
  ASSERT_EQ(buf2[1], 50);

  close_fd(rfd);
  close_fd(wfd);
}
