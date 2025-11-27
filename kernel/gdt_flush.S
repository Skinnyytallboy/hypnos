.global gdt_flush

gdt_flush:
    lgdt (%eax)

    mov $0x10, %ax        # data segment selector
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    ljmp $0x08, $reload_cs  # code segment selector

reload_cs:
    ret
