#include <span.h>
#include <string.h>

#include "address_space.h"
#include "elf.h"
#include "ktest.h"
#include "paging.h"
#include "pmm.h"
#include "process.h"
#include "scheduler.h"

// ===========================================================================
// AddressSpace
// ===========================================================================

TEST(address_space, create_and_destroy) {
  auto [phys, virt] = AddressSpace::create();
  ASSERT_NOT_NULL(virt);
  ASSERT_NE(phys, 0U);

  // Kernel PDEs (768-1023) should be copied from boot_page_directory.
  for (uint32_t i = AddressSpace::kKernelPdeStart; i < PAGES_PER_TABLE; ++i) {
    ASSERT_EQ(virt->entry[i].present, boot_page_directory.entry[i].present);
    if (virt->entry[i].present) {
      ASSERT_EQ(virt->entry[i].frame, boot_page_directory.entry[i].frame);
    }
  }

  // User PDEs (0-767) should be zeroed.
  for (uint32_t i = 0; i < AddressSpace::kKernelPdeStart; ++i) {
    ASSERT_EQ(virt->entry[i].present, 0U);
  }

  AddressSpace::destroy(virt, phys);
}

TEST(address_space, map_user_page) {
  auto [pd_phys, pd] = AddressSpace::create();
  paddr_t const page = kPmm.alloc();
  ASSERT_NE(page, 0U);

  AddressSpace::map(pd, 0x00400000, page, true, true);

  // Verify the PDE and PTE were created.
  uint32_t const pdi = 0x00400000 >> 22;
  ASSERT_EQ(pd->entry[pdi].present, 1U);
  ASSERT_EQ(pd->entry[pdi].user, 1U);

  AddressSpace::destroy(pd, pd_phys);
}

// ===========================================================================
// AddressSpace::copy
// ===========================================================================

TEST(address_space, copy_empty_has_kernel_pdes) {
  // An empty address space (no user pages) can be copied; the result is a
  // distinct page directory that still shares all kernel PDEs.
  auto [src_phys, src_pd] = AddressSpace::create();
  auto [dst_phys, dst_pd] = AddressSpace::copy(src_pd);

  ASSERT_NOT_NULL(dst_pd);
  ASSERT_NE(dst_phys, src_phys);

  for (uint32_t i = AddressSpace::kKernelPdeStart; i < PAGES_PER_TABLE; ++i) {
    ASSERT_EQ(dst_pd->entry[i].present, boot_page_directory.entry[i].present);
    if (dst_pd->entry[i].present) {
      ASSERT_EQ(dst_pd->entry[i].frame, boot_page_directory.entry[i].frame);
    }
  }

  AddressSpace::destroy(src_pd, src_phys);
  AddressSpace::destroy(dst_pd, dst_phys);
}

TEST(address_space, copy_propagates_user_mapping) {
  // A user page mapped in the source must appear at the same virtual address
  // in the copy, with the same access flags.
  auto [src_phys, src_pd] = AddressSpace::create();
  paddr_t const page = kPmm.alloc();
  ASSERT_NE(page, 0U);
  AddressSpace::map(src_pd, 0x00400000, page, /*writeable=*/true,
                    /*user=*/true);

  auto [dst_phys, dst_pd] = AddressSpace::copy(src_pd);
  ASSERT_NOT_NULL(dst_pd);

  ASSERT_TRUE(AddressSpace::is_user_mapped(dst_pd, 0x00400000, /*writeable=*/true));

  AddressSpace::destroy(src_pd, src_phys);
  AddressSpace::destroy(dst_pd, dst_phys);
}

TEST(address_space, copy_uses_distinct_physical_frames) {
  // The copied page must be backed by a different physical frame so writes
  // in one address space do not affect the other.
  auto [src_phys, src_pd] = AddressSpace::create();
  paddr_t const page = kPmm.alloc();
  ASSERT_NE(page, 0U);
  AddressSpace::map(src_pd, 0x00400000, page, /*writeable=*/true,
                    /*user=*/true);

  auto [dst_phys, dst_pd] = AddressSpace::copy(src_pd);
  ASSERT_NOT_NULL(dst_pd);

  const uint32_t pdi = 0x00400000 >> 22;
  const uint32_t pti = (0x00400000 >> 12) & 0x3FF;
  const auto& src_pde = src_pd->entry[pdi];
  const auto& dst_pde = dst_pd->entry[pdi];
  ASSERT_EQ(src_pde.present, 1U);
  ASSERT_EQ(dst_pde.present, 1U);

  const auto* src_pt =
      phys_to_virt(paddr_t{static_cast<uintptr_t>(src_pde.frame) << PAGE_OFFSET_BITS})
          .ptr<const PageTable>();
  const auto* dst_pt =
      phys_to_virt(paddr_t{static_cast<uintptr_t>(dst_pde.frame) << PAGE_OFFSET_BITS})
          .ptr<const PageTable>();

  ASSERT_NE(src_pt->entry[pti].frame, dst_pt->entry[pti].frame);

  AddressSpace::destroy(src_pd, src_phys);
  AddressSpace::destroy(dst_pd, dst_phys);
}

TEST(address_space, copy_replicates_page_data) {
  // Page contents from the source must be present verbatim in the copy.
  auto [src_phys, src_pd] = AddressSpace::create();
  paddr_t const page = kPmm.alloc();
  ASSERT_NE(page, 0U);

  auto* src_data = phys_to_virt(page).ptr<uint8_t>();
  for (uint32_t i = 0; i < PAGE_SIZE; ++i) {
    src_data[i] = static_cast<uint8_t>(i & 0xFF);
  }
  AddressSpace::map(src_pd, 0x00400000, page, /*writeable=*/true,
                    /*user=*/true);

  auto [dst_phys, dst_pd] = AddressSpace::copy(src_pd);
  ASSERT_NOT_NULL(dst_pd);

  const uint32_t pdi = 0x00400000 >> 22;
  const uint32_t pti = (0x00400000 >> 12) & 0x3FF;
  const auto& dst_pde = dst_pd->entry[pdi];
  ASSERT_EQ(dst_pde.present, 1U);

  const auto* dst_pt =
      phys_to_virt(paddr_t{static_cast<uintptr_t>(dst_pde.frame) << PAGE_OFFSET_BITS})
          .ptr<const PageTable>();
  const auto* dst_data =
      phys_to_virt(paddr_t{static_cast<uintptr_t>(dst_pt->entry[pti].frame) << PAGE_OFFSET_BITS})
          .ptr<const uint8_t>();

  ASSERT_EQ(memcmp(src_data, dst_data, PAGE_SIZE), 0);

  AddressSpace::destroy(src_pd, src_phys);
  AddressSpace::destroy(dst_pd, dst_phys);
}

// ===========================================================================
// Scheduler
// ===========================================================================

TEST(scheduler, current_not_null) {
  Process const* p = Scheduler::current();
  ASSERT_NOT_NULL(p);
  ASSERT_EQ(p->pid, 0U);  // idle process has PID 0
}

namespace {

// Build a minimal valid ELF32 i386 executable in a buffer.
// Contains a single PT_LOAD segment with no file data (just BSS).
struct TestElf {
  Elf::Header hdr;
  Elf::ProgramHeader phdr;
};

void make_test_elf(TestElf& elf, uint32_t entry) {
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

TEST(scheduler, create_process) {
  TestElf elf;
  make_test_elf(elf, 0x00400000);

  Process const* p = Scheduler::create_process(
      std::span<const uint8_t>{reinterpret_cast<const uint8_t*>(&elf), sizeof(elf)}, "test");
  ASSERT_NOT_NULL(p);
  ASSERT_EQ(p->pid, 1U);  // first user process
  ASSERT_EQ(p->state, ProcessState::Ready);
  ASSERT_NE(p->kernel_stack, nullptr);
  ASSERT_NE(p->page_directory_phys, 0U);
}

TEST(scheduler, create_multiple_processes) {
  TestElf elf;
  make_test_elf(elf, 0x00400000);
  const std::span<const uint8_t> data{reinterpret_cast<const uint8_t*>(&elf), sizeof(elf)};

  Process const* p1 = Scheduler::create_process(data, "test1");
  Process const* p2 = Scheduler::create_process(data, "test2");
  Process const* p3 = Scheduler::create_process(data, "test3");

  ASSERT_NOT_NULL(p1);
  ASSERT_NOT_NULL(p2);
  ASSERT_NOT_NULL(p3);

  ASSERT_NE(p1->pid, p2->pid);
  ASSERT_NE(p2->pid, p3->pid);
  ASSERT_TRUE(p3->pid > p1->pid);
}
