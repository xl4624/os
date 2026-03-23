/*
 * crt0.S -- Userspace C runtime entry point.
 *
 * Calls main(), then passes its return value to sys_exit.
 */

.set SYS_EXIT, 0

.section .text
.global _start
.extern main
.extern environ

_start:
    /* At entry, the stack looks like:
     *   [esp+0]              argc
     *   [esp+4]              argv[0]  (pointer to first arg string)
     *   ...
     *   [esp+4+argc*4]       NULL     (argv terminator)
     *   [esp+4+(argc+1)*4]   envp[0]  (pointer to first "NAME=VALUE" string)
     *   ...
     *                        NULL     (envp terminator)
     *
     * Load argc, argv, and envp, set the environ global, then call
     * main(argc, argv, envp).
     */
    mov  (%esp), %eax           /* eax = argc */
    lea  4(%esp), %ecx          /* ecx = &argv[0] */
    lea  4(%ecx,%eax,4), %edx   /* edx = &envp[0] = argv + (argc+1) */
    movl %edx, environ          /* environ = envp */
    push %edx                   /* push envp (3rd argument) */
    push %ecx                   /* push argv (2nd argument) */
    push %eax                   /* push argc (1st argument) */
    call main

    /* sys_exit(main return value) */
    mov %eax, %ebx
    mov $SYS_EXIT, %eax
    int $0x80
