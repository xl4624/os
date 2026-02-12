#include <string.h>

#include "address_space.h"
#include "elf.h"
#include "ktest.h"
#include "paging.h"

namespace {

    // Minimal valid ELF32 i386 executable: one PT_LOAD segment that is BSS-only
    // (filesz=0, memsz=PAGE_SIZE). Reused across validate and load tests.
    struct TestElf {
        Elf::Header hdr;
        Elf::ProgramHeader phdr;
    };

    // ELF with two PT_LOAD segments at separate virtual addresses.
    struct TwoSegElf {
        Elf::Header hdr;
        Elf::ProgramHeader phdr[2];
    };

    void make_valid_elf(TestElf &elf, uint32_t entry = 0x00400000) {
        memset(&elf, 0, sizeof(elf));

        elf.hdr.e_ident[Elf::kEiMag0] = Elf::kElfMag0;
        elf.hdr.e_ident[Elf::kEiMag1] = Elf::kElfMag1;
        elf.hdr.e_ident[Elf::kEiMag2] = Elf::kElfMag2;
        elf.hdr.e_ident[Elf::kEiMag3] = Elf::kElfMag3;
        elf.hdr.e_ident[Elf::kEiClass] = Elf::kElfClass32;
        elf.hdr.e_ident[Elf::kEiData]  = Elf::kElfData2Lsb;
        elf.hdr.e_type       = Elf::kEtExec;
        elf.hdr.e_machine    = Elf::kEm386;
        elf.hdr.e_version    = 1;
        elf.hdr.e_entry      = entry;
        elf.hdr.e_phoff      = sizeof(Elf::Header);
        elf.hdr.e_phentsize  = sizeof(Elf::ProgramHeader);
        elf.hdr.e_phnum      = 1;
        elf.hdr.e_ehsize     = sizeof(Elf::Header);

        elf.phdr.p_type   = Elf::kPtLoad;
        elf.phdr.p_offset = 0;
        elf.phdr.p_vaddr  = entry & ~(PAGE_SIZE - 1);
        elf.phdr.p_filesz = 0;
        elf.phdr.p_memsz  = PAGE_SIZE;
        elf.phdr.p_flags  = Elf::kPfR | Elf::kPfX;
        elf.phdr.p_align  = PAGE_SIZE;
    }

    void make_two_seg_elf(TwoSegElf &elf, uint32_t entry = 0x00400000,
                          uint32_t vaddr2 = 0x00500000) {
        memset(&elf, 0, sizeof(elf));

        elf.hdr.e_ident[Elf::kEiMag0] = Elf::kElfMag0;
        elf.hdr.e_ident[Elf::kEiMag1] = Elf::kElfMag1;
        elf.hdr.e_ident[Elf::kEiMag2] = Elf::kElfMag2;
        elf.hdr.e_ident[Elf::kEiMag3] = Elf::kElfMag3;
        elf.hdr.e_ident[Elf::kEiClass] = Elf::kElfClass32;
        elf.hdr.e_ident[Elf::kEiData]  = Elf::kElfData2Lsb;
        elf.hdr.e_type       = Elf::kEtExec;
        elf.hdr.e_machine    = Elf::kEm386;
        elf.hdr.e_version    = 1;
        elf.hdr.e_entry      = entry;
        elf.hdr.e_phoff      = sizeof(Elf::Header);
        elf.hdr.e_phentsize  = sizeof(Elf::ProgramHeader);
        elf.hdr.e_phnum      = 2;
        elf.hdr.e_ehsize     = sizeof(Elf::Header);

        elf.phdr[0].p_type   = Elf::kPtLoad;
        elf.phdr[0].p_offset = 0;
        elf.phdr[0].p_vaddr  = entry & ~(PAGE_SIZE - 1);
        elf.phdr[0].p_filesz = 0;
        elf.phdr[0].p_memsz  = PAGE_SIZE;
        elf.phdr[0].p_flags  = Elf::kPfR | Elf::kPfX;
        elf.phdr[0].p_align  = PAGE_SIZE;

        elf.phdr[1].p_type   = Elf::kPtLoad;
        elf.phdr[1].p_offset = 0;
        elf.phdr[1].p_vaddr  = vaddr2;
        elf.phdr[1].p_filesz = 0;
        elf.phdr[1].p_memsz  = PAGE_SIZE;
        elf.phdr[1].p_flags  = Elf::kPfR | Elf::kPfW;
        elf.phdr[1].p_align  = PAGE_SIZE;
    }

}  // namespace

// ---------------------------------------------------------------------------
// Elf::validate
// ---------------------------------------------------------------------------

TEST(elf, validate_rejects_too_small) {
    uint8_t tiny[4] = {0x7F, 'E', 'L', 'F'};
    ASSERT_FALSE(Elf::validate(tiny, sizeof(tiny)));
}

TEST(elf, validate_rejects_bad_magic) {
    TestElf elf;
    make_valid_elf(elf);
    elf.hdr.e_ident[Elf::kEiMag0] = 0x00;
    ASSERT_FALSE(Elf::validate(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf)));
}

TEST(elf, validate_rejects_non_32bit) {
    TestElf elf;
    make_valid_elf(elf);
    elf.hdr.e_ident[Elf::kEiClass] = 2;  // ELFCLASS64
    ASSERT_FALSE(Elf::validate(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf)));
}

TEST(elf, validate_rejects_big_endian) {
    TestElf elf;
    make_valid_elf(elf);
    elf.hdr.e_ident[Elf::kEiData] = 2;  // ELFDATA2MSB
    ASSERT_FALSE(Elf::validate(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf)));
}

TEST(elf, validate_rejects_non_executable) {
    TestElf elf;
    make_valid_elf(elf);
    elf.hdr.e_type = 3;  // ET_DYN
    ASSERT_FALSE(Elf::validate(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf)));
}

TEST(elf, validate_rejects_non_i386) {
    TestElf elf;
    make_valid_elf(elf);
    elf.hdr.e_machine = 62;  // EM_X86_64
    ASSERT_FALSE(Elf::validate(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf)));
}

TEST(elf, validate_rejects_no_program_headers) {
    TestElf elf;
    make_valid_elf(elf);
    elf.hdr.e_phnum = 0;
    ASSERT_FALSE(Elf::validate(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf)));
}

TEST(elf, validate_accepts_valid_elf) {
    TestElf elf;
    make_valid_elf(elf);
    ASSERT_TRUE(Elf::validate(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf)));
}

// ---------------------------------------------------------------------------
// Elf::load
// ---------------------------------------------------------------------------

TEST(elf, load_sets_entry_point) {
    TestElf elf;
    make_valid_elf(elf, 0x00401234);

    auto [pd_phys, pd] = AddressSpace::create();
    vaddr_t entry = 0;
    vaddr_t brk = 0;
    ASSERT_TRUE(Elf::load(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf),
                          pd, entry, brk));
    ASSERT_EQ(entry, 0x00401234u);
    AddressSpace::destroy(pd, pd_phys);
}

TEST(elf, load_sets_brk_page_aligned) {
    TestElf elf;
    make_valid_elf(elf, 0x00400000);

    auto [pd_phys, pd] = AddressSpace::create();
    vaddr_t entry = 0;
    vaddr_t brk = 0;
    ASSERT_TRUE(Elf::load(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf),
                          pd, entry, brk));
    // brk must be page-aligned.
    ASSERT_EQ(brk & (PAGE_SIZE - 1), 0u);
    // brk must be past the end of the loaded segment.
    ASSERT_TRUE(brk >= 0x00400000u + PAGE_SIZE);
    AddressSpace::destroy(pd, pd_phys);
}

TEST(elf, load_segment_is_user_mapped) {
    TestElf elf;
    make_valid_elf(elf, 0x00400000);

    auto [pd_phys, pd] = AddressSpace::create();
    vaddr_t entry = 0;
    vaddr_t brk = 0;
    ASSERT_TRUE(Elf::load(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf),
                          pd, entry, brk));
    // The segment starting at 0x00400000 must be user-accessible.
    ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
    AddressSpace::destroy(pd, pd_phys);
}

TEST(elf, load_rejects_kernel_space_segment) {
    TestElf elf;
    make_valid_elf(elf, 0x00400000);
    elf.phdr.p_vaddr = KERNEL_VMA;  // segment in kernel space

    auto [pd_phys, pd] = AddressSpace::create();
    vaddr_t entry = 0;
    vaddr_t brk = 0;
    ASSERT_FALSE(Elf::load(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf),
                           pd, entry, brk));
    AddressSpace::destroy(pd, pd_phys);
}

// A TestElf holds exactly one program header. Setting e_phnum=2 makes the
// computed ph_end exceed sizeof(TestElf), so validate must reject it.
TEST(elf, validate_rejects_phdrs_out_of_bounds) {
    TestElf elf;
    make_valid_elf(elf);
    elf.hdr.e_phnum = 2;
    ASSERT_FALSE(Elf::validate(reinterpret_cast<const uint8_t *>(&elf),
                               sizeof(elf)));
}

// A segment with p_filesz > 0 whose file data extends past the ELF buffer
// must be rejected.
TEST(elf, load_rejects_file_data_out_of_bounds) {
    TestElf elf;
    make_valid_elf(elf, 0x00400000);
    // Place file data so that p_offset + p_filesz > sizeof(elf).
    elf.phdr.p_offset = sizeof(elf) - 1;
    elf.phdr.p_filesz = 2;  // extends one byte past the end

    auto [pd_phys, pd] = AddressSpace::create();
    vaddr_t entry = 0;
    vaddr_t brk = 0;
    ASSERT_FALSE(Elf::load(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf),
                           pd, entry, brk));
    AddressSpace::destroy(pd, pd_phys);
}

// A PT_LOAD segment with memsz=0 must be silently skipped: the address range
// must not appear in the user address space.
TEST(elf, load_zero_memsz_segment_not_mapped) {
    TestElf elf;
    make_valid_elf(elf, 0x00400000);
    elf.phdr.p_memsz = 0;

    auto [pd_phys, pd] = AddressSpace::create();
    vaddr_t entry = 0;
    vaddr_t brk = 0;
    ASSERT_TRUE(Elf::load(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf),
                          pd, entry, brk));
    ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00400000,
                                              /*writeable=*/false));
    AddressSpace::destroy(pd, pd_phys);
}

TEST(elf, load_two_segments_both_mapped) {
    TwoSegElf elf;
    make_two_seg_elf(elf, 0x00400000, 0x00500000);

    auto [pd_phys, pd] = AddressSpace::create();
    vaddr_t entry = 0;
    vaddr_t brk = 0;
    ASSERT_TRUE(Elf::load(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf),
                          pd, entry, brk));
    ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000,
                                             /*writeable=*/false));
    ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00500000,
                                             /*writeable=*/false));
    AddressSpace::destroy(pd, pd_phys);
}

// brk must be past the end of the highest-address segment (0x00500000 + PAGE_SIZE).
TEST(elf, load_two_segments_brk_past_both) {
    TwoSegElf elf;
    make_two_seg_elf(elf, 0x00400000, 0x00500000);

    auto [pd_phys, pd] = AddressSpace::create();
    vaddr_t entry = 0;
    vaddr_t brk = 0;
    ASSERT_TRUE(Elf::load(reinterpret_cast<const uint8_t *>(&elf), sizeof(elf),
                          pd, entry, brk));
    ASSERT_TRUE(brk >= 0x00500000u + PAGE_SIZE);
    AddressSpace::destroy(pd, pd_phys);
}
