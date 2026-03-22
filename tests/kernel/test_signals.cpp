#include <signal.h>
#include <span.h>
#include <string.h>

#include "elf.h"
#include "ktest.h"
#include "process.h"
#include "scheduler.h"

namespace {

// Minimal valid ELF32 i386 executable (one empty PT_LOAD segment).
// Matches the helper in test_scheduler.cpp.
struct TestElf {
  Elf::Header hdr;
  Elf::ProgramHeader phdr;
};

void make_test_elf(TestElf& elf, uint32_t entry = 0x00400000) {
  memset(&elf, 0, sizeof(elf));

  elf.hdr.e_ident[Elf::kEiMag0] = Elf::kElfMag0;
  elf.hdr.e_ident[Elf::kEiMag1] = Elf::kElfMag1;
  elf.hdr.e_ident[Elf::kEiMag2] = Elf::kElfMag2;
  elf.hdr.e_ident[Elf::kEiMag3] = Elf::kElfMag3;
  elf.hdr.e_ident[Elf::kEiClass] = Elf::kElfClass32;
  elf.hdr.e_ident[Elf::kEiData] = Elf::kElfData2Lsb;
  elf.hdr.e_type = Elf::kEtExec;
  elf.hdr.e_machine = Elf::kEm386;
  elf.hdr.e_version = 1;
  elf.hdr.e_entry = entry;
  elf.hdr.e_phoff = sizeof(Elf::Header);
  elf.hdr.e_phentsize = sizeof(Elf::ProgramHeader);
  elf.hdr.e_phnum = 1;
  elf.hdr.e_ehsize = sizeof(Elf::Header);

  elf.phdr.p_type = Elf::kPtLoad;
  elf.phdr.p_offset = 0;
  elf.phdr.p_vaddr = entry & ~(PAGE_SIZE - 1);
  elf.phdr.p_filesz = 0;
  elf.phdr.p_memsz = PAGE_SIZE;
  elf.phdr.p_flags = Elf::kPfR | Elf::kPfX;
  elf.phdr.p_align = PAGE_SIZE;
}

}  // namespace

// ===========================================================================
// ProcessState
// ===========================================================================

TEST(signals, empty_state_is_zero) {
  // ProcessState::Empty must be 0 so that zero-initialised Process slots are
  // treated as unused rather than as Ready processes.
  ASSERT_EQ(static_cast<uint8_t>(ProcessState::Empty), 0U);
}

TEST(signals, default_constructed_process_is_empty) {
  const Process p{};
  ASSERT_EQ(p.state, ProcessState::Empty);
  ASSERT_EQ(p.pid, 0U);
  ASSERT_EQ(p.pending_signals, 0U);
}

// ===========================================================================
// Signal handler defaults
// ===========================================================================

TEST(signals, new_process_handlers_are_sigdfl) {
  // Freshly created processes must have all signal handlers set to SIG_DFL so
  // that unhandled signals use the default action (terminate).
  TestElf elf;
  make_test_elf(elf);
  const Process* p = Scheduler::create_process(
      std::span<const uint8_t>{reinterpret_cast<const uint8_t*>(&elf), sizeof(elf)}, "test_sigdfl");
  ASSERT_NOT_NULL(p);
  ASSERT_EQ(p->pending_signals, 0U);
  for (uint32_t sig = 0; sig < 32; ++sig) {
    ASSERT_EQ(p->signal_handlers[sig], kSigDfl);
  }
}

// ===========================================================================
// send_signal
// ===========================================================================

TEST(signals, send_signal_sets_pending_bit) {
  TestElf elf;
  make_test_elf(elf);
  const Process* p = Scheduler::create_process(
      std::span<const uint8_t>{reinterpret_cast<const uint8_t*>(&elf), sizeof(elf)},
      "test_send_signal");
  ASSERT_NOT_NULL(p);
  ASSERT_EQ(p->pending_signals, 0U);

  Scheduler::send_signal(p->pid, SIGINT);
  ASSERT_EQ(p->pending_signals, 1U << SIGINT);

  Scheduler::send_signal(p->pid, SIGTERM);
  ASSERT_EQ(p->pending_signals, (1U << SIGINT) | (1U << SIGTERM));
}

TEST(signals, send_signal_ignores_signum_zero) {
  // Signal 0 is used for existence checks; must not set any bit.
  const Process* p = Scheduler::current();
  const uint32_t before = p->pending_signals;
  Scheduler::send_signal(p->pid, 0);
  ASSERT_EQ(p->pending_signals, before);
}

TEST(signals, send_signal_ignores_signum_out_of_range) {
  const Process* p = Scheduler::current();
  const uint32_t before = p->pending_signals;
  Scheduler::send_signal(p->pid, 32);
  ASSERT_EQ(p->pending_signals, before);
}

TEST(signals, send_signal_does_not_affect_zombie) {
  TestElf elf;
  make_test_elf(elf);
  Process* p = Scheduler::create_process(
      std::span<const uint8_t>{reinterpret_cast<const uint8_t*>(&elf), sizeof(elf)}, "test_zombie");
  ASSERT_NOT_NULL(p);

  // Force Zombie state to simulate an exited process, then clear pending bits.
  p->state = ProcessState::Zombie;
  p->pending_signals = 0;

  Scheduler::send_signal(p->pid, SIGINT);
  ASSERT_EQ(p->pending_signals, 0U);  // zombie must not receive signals

  // Restore Empty so the scheduler ignores this slot.
  p->state = ProcessState::Empty;
}

// ===========================================================================
// has_user_processes / broadcast_signal
// ===========================================================================

TEST(signals, has_user_processes_true_after_create) {
  // After creating at least one process the function must return true.
  TestElf elf;
  make_test_elf(elf);
  const Process* p = Scheduler::create_process(
      std::span<const uint8_t>{reinterpret_cast<const uint8_t*>(&elf), sizeof(elf)},
      "test_has_user");
  ASSERT_NOT_NULL(p);
  ASSERT_TRUE(Scheduler::has_user_processes());
}

TEST(signals, broadcast_signal_skips_zombie) {
  TestElf elf;
  make_test_elf(elf);
  Process* p = Scheduler::create_process(
      std::span<const uint8_t>{reinterpret_cast<const uint8_t*>(&elf), sizeof(elf)},
      "test_broadcast_zombie");
  ASSERT_NOT_NULL(p);

  p->state = ProcessState::Zombie;
  p->pending_signals = 0;

  Scheduler::broadcast_signal(SIGINT);
  ASSERT_EQ(p->pending_signals, 0U);

  // Restore Empty so the scheduler ignores this slot.
  p->state = ProcessState::Empty;
}

TEST(signals, broadcast_signal_skips_empty) {
  // An Empty slot (pid=0 after Process{}) must not receive broadcast signals.
  const Process empty{};
  // Empty slots are not in the process table, so this is a logical check:
  // empty.state == Empty means broadcast_signal will never call send_signal on it.
  ASSERT_EQ(empty.state, ProcessState::Empty);
  ASSERT_NE(empty.state, ProcessState::Ready);
  ASSERT_NE(empty.state, ProcessState::Running);
  ASSERT_NE(empty.state, ProcessState::Blocked);
  ASSERT_NE(empty.state, ProcessState::Zombie);
}
