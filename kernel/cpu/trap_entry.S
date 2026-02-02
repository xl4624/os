/*
 * Interrupt and syscall entry points.
 *
 * Both timer_entry (IRQ 0, vector 32) and syscall_entry (int 0x80) need
 * identical register save/restore with full control over the stack frame
 * for context switching. They share a common body via the TRAP_ENTRY macro
 * and differ only in which C++ dispatch function they call.
 *
 * The saved register layout matches TrapFrame (process.h):
 *
 *   [ESP+0]  edi   -+
 *   [ESP+4]  esi    |
 *   [ESP+8]  ebp    | pushal
 *   [ESP+12] (esp)  | (ignored by popal)
 *   [ESP+16] ebx    |
 *   [ESP+20] edx    |
 *   [ESP+24] ecx    |
 *   [ESP+28] eax   -+
 *   [ESP+32] ds    -+
 *   [ESP+36] es     | segment registers
 *   [ESP+40] fs     |
 *   [ESP+44] gs    -+
 *   [ESP+48] eip   -+
 *   [ESP+52] cs     | pushed by CPU
 *   [ESP+56] eflags |
 *   [ESP+60] esp    | (user, only on ring change)
 *   [ESP+64] ss    -+ (user, only on ring change)
 *
 * Each dispatch function receives the current kernel ESP (pointing to the
 * TrapFrame) and returns the ESP to restore. If a context switch occurred,
 * the returned ESP points to a different process's TrapFrame.
 */

.macro TRAP_ENTRY dispatch
    push %gs
    push %fs
    push %es
    push %ds
    pushal

    mov $0x10, %ax          /* KERNEL_DATA_SELECTOR */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push %esp
    call \dispatch
    mov %eax, %esp          /* may be a new process's stack */

    popal
    pop %ds
    pop %es
    pop %fs
    pop %gs
    iret
.endm

/* Timer IRQ (vector 32). Registered in IDT by Scheduler::init(). */
.global timer_entry
.type timer_entry, @function
timer_entry:
    TRAP_ENTRY timer_dispatch

/* System call (int 0x80). Registered in IDT by Syscall::init(). */
.global syscall_entry
.type syscall_entry, @function
syscall_entry:
    TRAP_ENTRY syscall_dispatch
