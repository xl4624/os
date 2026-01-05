/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

KERNEL_VMA = 0xC0000000;

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
    /* Boot section: linked and loaded at physical addresses.
       Must be early in the binary so GRUB finds the multiboot header
       within the first 8 KiB. */
    . = 2M;

    .boot ALIGN(4K) :
    {
        *(.multiboot)
        *(.boot)
    }

    /* Transition to virtual address space. All subsequent sections are
       linked at virtual addresses (0xC0000000+) but loaded at physical
       addresses (via AT). */
    . = ALIGN(4K) + KERNEL_VMA;

    kernel_start = .;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA)
    {
        *(.text)
    }

    /* C++ global constructors/destructors (crti.o, crtbegin.o, crtend.o, crtn.o) */
    .init ALIGN(4) : AT(ADDR(.init) - KERNEL_VMA)
    {
        KEEP(*(.init))
    }

    .fini ALIGN(4) : AT(ADDR(.fini) - KERNEL_VMA)
    {
        KEEP(*(.fini))
    }

    /* Read-only data. */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA)
    {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA)
    {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA)
    {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4K);
    kernel_end = .;
}
