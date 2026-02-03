/*
 * crtn.S -- C runtime initialization epilogue.
 *
 * Provides the closing halves of _init and _fini (stack teardown and
 * ret). Paired with crti.S; the linker sandwiches crtbegin/crtend
 * sections between them.
 */

.section .init
   /* gcc will nicely put the contents of crtend.o's .init section here. */
   pop %ebp
   ret

.section .fini
   /* gcc will nicely put the contents of crtend.o's .fini section here. */
   pop %ebp
   ret

