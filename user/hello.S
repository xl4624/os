/*
 * hello.S -- Minimal ring-3 test program.
 *
 * Writes "Hello from userspace!\n" to stdout via sys_write, then exits
 * cleanly via sys_exit. The program is position-dependent: it expects
 * to be loaded at USER_CODE_VA (currently 0x00800000).
 *
 * Syscall ABI (int 0x80):
 *   EAX = syscall number
 *   EBX = arg1,  ECX = arg2,  EDX = arg3,  ESI = arg4,  EDI = arg5
 *   Return value in EAX.
 *
 * TODO: Build this with the cross-toolchain into a flat binary and embed
 * it via objcopy, replacing the hand-encoded byte array.
 * TODO: Make position-independent once proper ELF loading exists.
 */

.set SYS_EXIT,  0
.set SYS_WRITE, 2

.section .text
.global _start

_start:
    /* sys_write(fd=1, buf=msg, count=msg_len) */
    mov $SYS_WRITE, %eax
    mov $1, %ebx
    mov $msg, %ecx
    mov $msg_len, %edx
    int $0x80

    /* sys_exit(code=0) */
    mov $SYS_EXIT, %eax
    xor %ebx, %ebx
    int $0x80

msg:
    .ascii "Hello from userspace!\n"
.set msg_len, . - msg
