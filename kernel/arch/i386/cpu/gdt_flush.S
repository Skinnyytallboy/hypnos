.global gdt_flush

gdt_flush:
    # get argument (pointer to gdt_ptr) from stack
    mov 4(%esp), %eax
    lgdt (%eax)

    # reload segment registers
    mov $0x10, %ax          # data segment selector (2nd descriptor)
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # far jump to reload CS
    ljmp $0x08, $1f         # code segment selector (1st descriptor)

1:
    ret
