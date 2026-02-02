/*
 * System call entry point for int 0x80.
 *
 * On entry from ring 3, the CPU has already pushed SS, ESP, EFLAGS, CS, EIP
 * onto the kernel stack (via TSS.esp0). We save segment and general-purpose
 * registers so the C dispatcher can read syscall number (EAX) and arguments
 * (EBX, ECX, EDX, ESI, EDI). The return value is placed in EAX by the
 * dispatcher, and we restore everything before iret back to user mode.
 *
 * Stack layout (SyscallRegisters struct, low address at top):
 *   [ESP+0]  edi   -+
 *   [ESP+4]  esi    |
 *   [ESP+8]  ebp    | pushal
 *   [ESP+12] (esp)  | (ignored by popal)
 *   [ESP+16] ebx    |
 *   [ESP+20] edx    |
 *   [ESP+24] ecx    |
 *   [ESP+28] eax   -+
 *   [ESP+32] ds     -+
 *   [ESP+36] es      | segment registers
 *   [ESP+40] fs      | (ds pushed last = lowest addr)
 *   [ESP+44] gs     -+ (gs pushed first = highest addr)
 *   [ESP+48] eip   -+
 *   [ESP+52] cs     | pushed by CPU
 *   [ESP+56] eflags |
 *   [ESP+60] esp    | (user)
 *   [ESP+64] ss    -+ (user)
 */

.global syscall_entry
.type syscall_entry, @function

syscall_entry:
    /* Save segment registers. */
    push %gs
    push %fs
    push %es
    push %ds

    /* Save all general-purpose registers. */
    pushal

    /* Load kernel data segment into ds, es, fs, gs. */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    /* Pass pointer to SyscallRegisters as argument. */
    push %esp
    call syscall_dispatch
    add $4, %esp

    /* Restore general-purpose registers. */
    popal

    /* Restore segment registers. */
    pop %ds
    pop %es
    pop %fs
    pop %gs

    iret
