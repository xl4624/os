#include <string.h>
#include <sys/syscall.h>

#include "file.h"
#include "ktest.h"
#include "process.h"
#include "scheduler.h"
#include "syscall.h"

// ---------------------------------------------------------------------------
// fd_init_stdio
// ---------------------------------------------------------------------------

TEST(fd, init_sets_stdio) {
  FileDescription* fds[kMaxFds];
  fd_init_stdio(fds);

  ASSERT_NOT_NULL(fds[0]);
  ASSERT_NOT_NULL(fds[1]);
  ASSERT_NOT_NULL(fds[2]);
  ASSERT_EQ(fds[0]->type, FileType::TerminalRead);
  ASSERT_EQ(fds[1]->type, FileType::TerminalWrite);
  ASSERT_EQ(fds[2]->type, FileType::TerminalWrite);

  // fds 1 and 2 share the same terminal write description.
  ASSERT(fds[1] == fds[2]);

  for (uint32_t i = 3; i < kMaxFds; ++i) {
    ASSERT_NULL(fds[i]);
  }
}

// ---------------------------------------------------------------------------
// fd_alloc
// ---------------------------------------------------------------------------

TEST(fd, alloc_returns_lowest_free) {
  FileDescription* fds[kMaxFds];
  fd_init_stdio(fds);

  int32_t slot = fd_alloc(fds);
  ASSERT_EQ(slot, 3);
}

TEST(fd, alloc_after_close_reuses_slot) {
  FileDescription* fds[kMaxFds];
  fd_init_stdio(fds);

  // "Close" fd 1 by nulling it.
  fds[1] = nullptr;

  int32_t slot = fd_alloc(fds);
  ASSERT_EQ(slot, 1);
}

TEST(fd, alloc_full_returns_negative) {
  FileDescription* fds[kMaxFds];
  FileDescription dummy = {FileType::TerminalRead, 1, nullptr};

  for (uint32_t i = 0; i < kMaxFds; ++i) {
    fds[i] = &dummy;
  }

  int32_t slot = fd_alloc(fds);
  ASSERT_EQ(slot, -1);
}

TEST(fd, alloc_from_skips_lower) {
  FileDescription* fds[kMaxFds];
  fd_init_stdio(fds);

  int32_t slot = fd_alloc_from(fds, 5);
  ASSERT_EQ(slot, 5);
}

// ---------------------------------------------------------------------------
// SYS_WRITE through fd table
// ---------------------------------------------------------------------------

TEST(fd, write_bad_fd) {
  TrapFrame frame = {};
  frame.eax = SYS_WRITE;
  frame.ebx = 99;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

TEST(fd, write_null_fd_slot) {
  Process* proc = Scheduler::current();
  FileDescription* saved = proc->fds[5];
  proc->fds[5] = nullptr;

  TrapFrame frame = {};
  frame.eax = SYS_WRITE;
  frame.ebx = 5;
  frame.ecx = 0;
  frame.edx = 0;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));

  proc->fds[5] = saved;
}

// ---------------------------------------------------------------------------
// SYS_CLOSE
// ---------------------------------------------------------------------------

TEST(fd, close_valid_fd) {
  Process* proc = Scheduler::current();

  auto* desc = new FileDescription{FileType::PipeRead, 1, nullptr};

  int32_t slot = fd_alloc(proc->fds);
  ASSERT(slot >= 0);
  proc->fds[slot] = desc;

  TrapFrame frame = {};
  frame.eax = SYS_CLOSE;
  frame.ebx = static_cast<uint32_t>(slot);
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));

  ASSERT_EQ(frame.eax, 0u);
  ASSERT_NULL(proc->fds[slot]);
}

TEST(fd, close_invalid_fd) {
  TrapFrame frame = {};
  frame.eax = SYS_CLOSE;
  frame.ebx = 99;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}

// ---------------------------------------------------------------------------
// SYS_DUP2
// ---------------------------------------------------------------------------

TEST(fd, dup2_copies_fd) {
  Process* proc = Scheduler::current();

  auto* desc = new FileDescription{FileType::PipeRead, 1, nullptr};

  int32_t slot = fd_alloc(proc->fds);
  ASSERT(slot >= 0);
  proc->fds[slot] = desc;

  TrapFrame frame = {};
  frame.eax = SYS_DUP2;
  frame.ebx = static_cast<uint32_t>(slot);
  frame.ecx = static_cast<uint32_t>(slot) + 1;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));

  uint32_t newfd = static_cast<uint32_t>(slot) + 1;
  ASSERT_EQ(frame.eax, newfd);
  ASSERT(proc->fds[newfd] == desc);
  ASSERT_EQ(desc->ref_count, 2u);

  // Cleanup: close both fds.
  proc->fds[slot] = nullptr;
  proc->fds[newfd] = nullptr;
  delete desc;
}

TEST(fd, dup2_same_fd) {
  Process* proc = Scheduler::current();

  TrapFrame frame = {};
  frame.eax = SYS_DUP2;
  frame.ebx = 0;  // stdin
  frame.ecx = 0;  // same
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));

  ASSERT_EQ(frame.eax, 0u);
  ASSERT_NOT_NULL(proc->fds[0]);
}

TEST(fd, dup2_invalid_oldfd) {
  TrapFrame frame = {};
  frame.eax = SYS_DUP2;
  frame.ebx = 99;
  frame.ecx = 3;
  syscall_dispatch(reinterpret_cast<uint32_t>(&frame));
  ASSERT_EQ(frame.eax, static_cast<uint32_t>(-1));
}
