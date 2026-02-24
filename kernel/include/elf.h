#pragma once

#include <stddef.h>
#include <stdint.h>

#include "paging.h"

/*
 * ELF32 type definitions and loader API.
 *
 * Supports loading statically-linked ELF32 executables for i386 into a
 * user-mode address space. Only PT_LOAD segments are processed; all other
 * program header types are ignored.
 */

namespace Elf {

// ELF identification indices.
static constexpr size_t kEiMag0 = 0;
static constexpr size_t kEiMag1 = 1;
static constexpr size_t kEiMag2 = 2;
static constexpr size_t kEiMag3 = 3;
static constexpr size_t kEiClass = 4;
static constexpr size_t kEiData = 5;
static constexpr size_t kEiNident = 16;

// ELF magic bytes.
static constexpr uint8_t kElfMag0 = 0x7F;
static constexpr uint8_t kElfMag1 = 'E';
static constexpr uint8_t kElfMag2 = 'L';
static constexpr uint8_t kElfMag3 = 'F';

// e_ident[EI_CLASS] values.
static constexpr uint8_t kElfClass32 = 1;

// e_ident[EI_DATA] values.
static constexpr uint8_t kElfData2Lsb = 1;  // little-endian

// e_type values.
static constexpr uint16_t kEtExec = 2;

// e_machine values.
static constexpr uint16_t kEm386 = 3;

// Program header p_type values.
static constexpr uint32_t kPtLoad = 1;

// Program header p_flags bits.
static constexpr uint32_t kPfX = 0x1;
static constexpr uint32_t kPfW = 0x2;
static constexpr uint32_t kPfR = 0x4;

// ELF32 file header.
struct Header {
  uint8_t e_ident[kEiNident];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} __attribute__((packed));

static_assert(sizeof(Header) == 52, "ELF32 header must be 52 bytes");

// ELF32 program header.
struct ProgramHeader {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
} __attribute__((packed));

static_assert(sizeof(ProgramHeader) == 32, "ELF32 program header must be 32 bytes");

// Validate that the buffer contains a well-formed ELF32 i386 executable.
[[nodiscard]]
bool validate(const uint8_t* data, size_t len);

// Load PT_LOAD segments from an ELF32 executable into the given page
// directory. On success, sets entry_out to e_entry and brk_out to the
// page-aligned end of the highest loaded segment (initial heap break).
[[nodiscard]]
bool load(const uint8_t* elf_data, size_t elf_len, PageTable* pd, vaddr_t& entry_out,
          vaddr_t& brk_out);

}  // namespace Elf
