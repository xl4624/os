/*
 * crti.S -- C runtime initialization prologue.
 *
 * Provides the opening halves of _init and _fini. The linker places
 * crtbegin.o's .init/.fini sections between crti.o and crtn.o, so
 * global constructors and destructors are called in the right order.
 */

.section .init
.global _init
_init:
   push %ebp
   mov %esp, %ebp
   /* gcc will nicely put the contents of crtbegin.o's .init section here. */

.section .fini
.global _fini
_fini:
   push %ebp
   mov %esp, %ebp
   /* gcc will nicely put the contents of crtbegin.o's .fini section here. */

