/*
 * crt0.S -- Userspace C runtime entry point.
 *
 * Calls main(), then passes its return value to sys_exit.
 */

.set SYS_EXIT, 0

.section .text
.global _start
.extern main

_start:
    call main

    /* sys_exit(main return value) */
    mov %eax, %ebx
    mov $SYS_EXIT, %eax
    int $0x80
