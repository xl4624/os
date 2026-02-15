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
    /* At entry, the stack looks like:
     *   [esp+0]  argc
     *   [esp+4]  argv[0]  (pointer to program name string)
     *   [esp+8]  NULL     (argv terminator)
     *
     * Load argc and argv, then call main(argc, argv).
     */
    mov  (%esp), %eax      /* eax = argc */
    lea  4(%esp), %ecx     /* ecx = argv (pointer to argv[0]) */
    push %ecx              /* push argv (2nd argument) */
    push %eax              /* push argc (1st argument) */
    call main

    /* sys_exit(main return value) */
    mov %eax, %ebx
    mov $SYS_EXIT, %eax
    int $0x80
