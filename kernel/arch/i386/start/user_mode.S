    .global switch_to_user_mode
    .global user_stack
    .global user_stack_top
    .extern user_program_main

    .equ USER_CODE_SEL, 0x1B
    .equ USER_DATA_SEL, 0x23

switch_to_user_mode:
    cli
    mov $USER_DATA_SEL, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov $user_stack_top, %eax

    pushl $USER_DATA_SEL
    pushl %eax
    pushfl
    pushl $USER_CODE_SEL
    pushl $user_program_main
    iret

    .align 16
user_stack:
    .space 4096
user_stack_top:
