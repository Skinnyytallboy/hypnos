# Multiboot v1 header + 32-bit entry point

.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

    .section .text
    .align 4

    # ---- Multiboot header (must be in first 8KiB of .text) ----
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

    .globl _start
    .type _start, @function

_start:
    cli
    mov $stack_top, %esp
    call kernel_main

.hang:
    hlt
    jmp .hang

    .section .bss
    .align 16
stack_bottom:
    .skip 16384           # 16 KB stack
stack_top:
