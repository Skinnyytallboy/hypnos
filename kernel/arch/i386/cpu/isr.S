# kernel/arch/i386/cpu/isr.S
# CPU exception stubs 0–31 + INT 0x80 syscall stub + idt_load

# -------- CPU exceptions 0–31 --------

.macro ISR n
    .global isr\n
    .extern isr_handler\n

isr\n:
    cli
    pusha

    # Call C handler isr_handler<n>()
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

.global isr80_syscall
.extern syscall_handler

isr80_syscall:
    cli

    # Save segment registers
    push %ds
    push %es
    push %fs
    push %gs

    # Preserve EAX (syscall number) while we load kernel data selector
    push %eax
    mov $0x10, %ax         # kernel data selector
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    pop %eax               # restore syscall number

    # Push args: syscall_handler(eax, ebx, ecx, edx)
    push %edx
    push %ecx
    push %ebx
    push %eax
    call syscall_handler
    add $16, %esp          # pop 4 args

    # return value already in eax

    pop %gs
    pop %fs
    pop %es
    pop %ds

    sti
    iret

# -------- idt_load helper --------

.global idt_load
idt_load:
    mov 4(%esp), %eax
    lidt (%eax)
    ret
