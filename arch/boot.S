/* Higher-half kernel boot code for i686.
   GRUB loads the kernel at physical 2 MiB. This code sets up initial page
   tables that identity-map and higher-half-map the first 8 MiB, enables
   paging, then jumps to the kernel's virtual address in the higher half. */

.set KERNEL_VMA,  0xC0000000
.set ALIGN,       1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,     1<<1             /* provide memory map */
.set FLAGS,       ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,       0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM,    -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/*
Multiboot header - must be within the first 8 KiB of the kernel binary.
Placed in .multiboot section so the linker script can force it first.
*/
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* ================================================================
   Data stored in .data (linked at virtual addresses).
   Boot code accesses these via (symbol - KERNEL_VMA).
   ================================================================ */
.section .data
.align 4
.global mboot_magic
.global mboot_info
mboot_magic:
    .long 0
mboot_info:
    .long 0

/* ================================================================
   Stack - 16 KiB in .bss (virtual addresses).
   ================================================================ */
.section .bss
.align 16
stack_bottom:
.skip 16384
.global stack_top
stack_top:

/* ================================================================
   Boot page tables - 3 pages in .bss (virtual addresses).
   Boot code accesses via (symbol - KERNEL_VMA) to get physical addr.
   ================================================================ */
.align 4096
.global boot_page_directory
boot_page_directory:
    .skip 4096
boot_page_table0:
    .skip 4096
boot_page_table1:
    .skip 4096

/* ================================================================
   Boot code - linked and loaded at physical addresses (in .boot).
   Runs with paging DISABLED.
   ================================================================ */
.section .boot, "ax"
.global _start
.type _start, @function
_start:
    /*
    Save multiboot info in callee-saved registers.
    %eax = magic number, %ebx = multiboot info pointer.
    We cannot store to globals yet because they are at virtual addresses
    and paging is not enabled.
    */
    mov %eax, %edi
    mov %ebx, %esi

    /* ---- Zero boot page tables (3 pages = 3072 dwords) ---- */
    mov $(boot_page_directory - KERNEL_VMA), %eax
    xor %ecx, %ecx
.Lzero:
    movl $0, (%eax, %ecx, 4)
    inc %ecx
    cmp $(3 * 1024), %ecx
    jl .Lzero

    /* ---- Fill boot_page_table0: map physical 0x00000000–0x003FFFFF ---- */
    mov $(boot_page_table0 - KERNEL_VMA), %eax
    xor %ecx, %ecx
    mov $0x003, %edx                /* present (1) + writable (2) */
.Lfill0:
    mov %edx, (%eax, %ecx, 4)
    add $0x1000, %edx
    inc %ecx
    cmp $1024, %ecx
    jl .Lfill0

    /* ---- Fill boot_page_table1: map physical 0x00400000–0x007FFFFF ---- */
    mov $(boot_page_table1 - KERNEL_VMA), %eax
    xor %ecx, %ecx
    mov $0x400003, %edx             /* 4 MiB + present + writable */
.Lfill1:
    mov %edx, (%eax, %ecx, 4)
    add $0x1000, %edx
    inc %ecx
    cmp $1024, %ecx
    jl .Lfill1

    /* ---- Set up page directory entries ---- */
    mov $(boot_page_directory - KERNEL_VMA), %eax

    /* PDE[0]: identity-map 0x00000000–0x003FFFFF (temporary) */
    mov $(boot_page_table0 - KERNEL_VMA + 0x003), %edx
    mov %edx, 0(%eax)

    /* PDE[1]: identity-map 0x00400000–0x007FFFFF (temporary) */
    mov $(boot_page_table1 - KERNEL_VMA + 0x003), %edx
    mov %edx, 4(%eax)

    /* PDE[768]: higher-half map 0xC0000000–0xC03FFFFF → physical 0–4M */
    mov $(boot_page_table0 - KERNEL_VMA + 0x003), %edx
    mov %edx, (768*4)(%eax)

    /* PDE[769]: higher-half map 0xC0400000–0xC07FFFFF → physical 4–8M */
    mov $(boot_page_table1 - KERNEL_VMA + 0x003), %edx
    mov %edx, (769*4)(%eax)

    /* ---- Enable paging ---- */
    mov $(boot_page_directory - KERNEL_VMA), %eax
    mov %eax, %cr3

    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    /* ---- Jump to higher half ---- */
    /* _higher_half is in .text, linked at a virtual address (0xC0......).
       After enabling paging, both identity and higher-half mappings are
       active, so the jump target is valid. */
    lea _higher_half, %eax
    jmp *%eax

/* ================================================================
   Higher-half entry point - in .text, linked at virtual addresses.
   Runs with paging ENABLED.
   ================================================================ */
.section .text
_higher_half:
    /* Remove the temporary identity mapping */
    movl $0, boot_page_directory + 0
    movl $0, boot_page_directory + 4

    /* Flush TLB for the identity-mapped pages */
    invlpg (0)
    invlpg (0x400000)

    /* Set up the kernel stack (virtual address) */
    mov $stack_top, %esp

    /* Store multiboot info into globals (now at virtual addresses) */
    mov %edi, mboot_magic
    mov %esi, mboot_info

    /* Call C++ global constructors */
    call _init

    /* Kernel early init: GDT, IDT, interrupts */
    call kernel_init

    /* Enter high-level kernel */
    call kernel_main

    /* Halt if kernel_main returns */
    cli
1:  hlt
    jmp 1b

.size _higher_half, . - _higher_half
