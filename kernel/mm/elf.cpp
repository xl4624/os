#include "elf.h"

#include <stdio.h>
#include <string.h>

#include "address_space.h"
#include "pmm.h"

namespace Elf {

    bool validate(const uint8_t *data, size_t len) {
        if (len < sizeof(Header)) {
            printf("ELF: too small (%u bytes)\n", static_cast<unsigned>(len));
            return false;
        }

        const auto *hdr = reinterpret_cast<const Header *>(data);

        if (hdr->e_ident[kEiMag0] != kElfMag0
            || hdr->e_ident[kEiMag1] != kElfMag1
            || hdr->e_ident[kEiMag2] != kElfMag2
            || hdr->e_ident[kEiMag3] != kElfMag3) {
            printf("ELF: bad magic\n");
            return false;
        }

        if (hdr->e_ident[kEiClass] != kElfClass32) {
            printf("ELF: not 32-bit\n");
            return false;
        }

        if (hdr->e_ident[kEiData] != kElfData2Lsb) {
            printf("ELF: not little-endian\n");
            return false;
        }

        if (hdr->e_type != kEtExec) {
            printf("ELF: not an executable (type=%u)\n", hdr->e_type);
            return false;
        }

        if (hdr->e_machine != kEm386) {
            printf("ELF: not i386 (machine=%u)\n", hdr->e_machine);
            return false;
        }

        if (hdr->e_phoff == 0 || hdr->e_phnum == 0) {
            printf("ELF: no program headers\n");
            return false;
        }

        const size_t ph_end =
            hdr->e_phoff + static_cast<size_t>(hdr->e_phnum) * hdr->e_phentsize;
        if (ph_end > len) {
            printf("ELF: program headers extend past end of file\n");
            return false;
        }

        return true;
    }

    bool load(const uint8_t *elf_data, size_t elf_len, PageTable *pd,
              vaddr_t &entry_out, vaddr_t &brk_out) {
        if (!validate(elf_data, elf_len)) {
            return false;
        }

        const auto *hdr = reinterpret_cast<const Header *>(elf_data);
        entry_out = hdr->e_entry;
        vaddr_t highest_end = 0;

        for (uint16_t i = 0; i < hdr->e_phnum; ++i) {
            const auto *ph = reinterpret_cast<const ProgramHeader *>(
                elf_data + hdr->e_phoff
                + static_cast<size_t>(i) * hdr->e_phentsize);

            if (ph->p_type != kPtLoad) {
                continue;
            }

            if (ph->p_memsz == 0) {
                continue;
            }

            // Validate that file data is within the ELF buffer.
            if (ph->p_filesz > 0 && (ph->p_offset + ph->p_filesz > elf_len)) {
                printf("ELF: segment %u file data out of bounds\n", i);
                return false;
            }

            // Reject segments that overlap the kernel's virtual space.
            if (ph->p_vaddr >= KERNEL_VMA) {
                printf("ELF: segment %u vaddr 0x%08x in kernel space\n", i,
                       ph->p_vaddr);
                return false;
            }

            const vaddr_t seg_start = ph->p_vaddr & ~(PAGE_SIZE - 1);
            const vaddr_t seg_end =
                (ph->p_vaddr + ph->p_memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

            if (seg_end > highest_end) {
                highest_end = seg_end;
            }

            for (vaddr_t va = seg_start; va < seg_end; va += PAGE_SIZE) {
                paddr_t phys = kPmm.alloc();
                if (phys == 0) {
                    printf("ELF: out of physical memory\n");
                    return false;
                }

                AddressSpace::map(pd, va, phys,
                                  /*writeable=*/true, /*user=*/true);

                auto *page = phys_to_virt(phys).ptr<uint8_t>();

                // Determine how much of this page overlaps with file data.
                size_t filled = 0;
                if (va.raw() < ph->p_vaddr + ph->p_filesz
                    && va.raw() + PAGE_SIZE > ph->p_vaddr) {
                    // Byte range within the segment that this page covers.
                    const size_t seg_offset =
                        (va.raw() > ph->p_vaddr) ? va.raw() - ph->p_vaddr : 0;
                    const size_t page_start =
                        (ph->p_vaddr > va.raw()) ? ph->p_vaddr - va.raw() : 0;
                    size_t copy_len = PAGE_SIZE - page_start;
                    if (seg_offset + copy_len > ph->p_filesz) {
                        copy_len = (ph->p_filesz > seg_offset)
                                       ? ph->p_filesz - seg_offset
                                       : 0;
                    }

                    if (page_start > 0) {
                        memset(page, 0, page_start);
                    }
                    if (copy_len > 0) {
                        memcpy(page + page_start,
                               elf_data + ph->p_offset + seg_offset, copy_len);
                    }
                    filled = page_start + copy_len;
                }

                if (filled < PAGE_SIZE) {
                    memset(page + filled, 0, PAGE_SIZE - filled);
                }
            }
        }

        brk_out = highest_end;
        return true;
    }

}  // namespace Elf
