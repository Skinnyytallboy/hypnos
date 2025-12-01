# kernel/arch/i386/cpu/isr.S
# CPU exception stubs 0–31 + INT 0x80 syscall stub + idt_load

# -------- CPU exceptions 0–31 --------

.macro ISR n
    .global isr\n
    .extern isr_handler\n

isr\n:
    cli
    pusha
    call isr_handler\n
    popa
    sti
    iret
.endm

ISR 0
ISR 1
ISR 2
ISR 3
ISR 4
ISR 5
ISR 6
ISR 7
ISR 8
ISR 9
ISR 10
ISR 11
ISR 12
ISR 13
ISR 14
ISR 15
ISR 16
ISR 17
ISR 18
ISR 19
ISR 20
ISR 21
ISR 22
ISR 23
ISR 24
ISR 25
ISR 26
ISR 27
ISR 28
ISR 29
ISR 30
ISR 31

# -------- INT 0x80 syscall stub --------
# User-side ABI:
#   eax = syscall number
#   ebx = arg1
#   ecx = arg2
#   edx = arg3
#
# C handler: uint32_t syscall_handler(uint32_t num, uint32_t a1,
#                                     uint32_t a2, uint32_t a3);
# Return value in eax.

.global isr80_syscall
.extern syscall_handler

isr80_syscall:
    # Save segment registers and switch to kernel data segment
    push %ds
    push %es
    mov $0x10, %ax          # kernel data selector
    mov %ax, %ds
    mov %ax, %es

    # Save callee-saved GPRs we must preserve for user mode
    push %ebp
    push %edi
    push %esi
    push %ebx

    # Build C-call stack: (a3, a2, a1, num)
    push %edx               # a3
    push %ecx               # a2
    push %ebx               # a1
    push %eax               # num
    call syscall_handler
    add $16, %esp           # pop args

    # eax now holds return value

    # Restore callee-saved regs
    pop %ebx
    pop %esi
    pop %edi
    pop %ebp

    # Restore original segment registers
    pop %es
    pop %ds

    iret

# -------- idt_load helper --------

.global idt_load
idt_load:
    mov 4(%esp), %eax
    lidt (%eax)
    ret
