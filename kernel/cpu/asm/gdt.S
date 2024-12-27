.section .text
.global load_gdt
.extern gdtp
load_gdt:
    lgdt (gdtp)
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    # Far jump to reload CS (code segment)
    # 0x08 is offset into GDT for code segment (2nd entry, so 8 bytes from start)
    ljmp $0x08, $.reload
.reload:
    ret

