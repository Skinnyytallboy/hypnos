#include "syscall.h"
#include "arch/i386/cpu/idt.h"
#include "console.h"

extern void isr80_syscall(void);

static void print_uint(uint32_t v)
{
    char buf[16];
    int i = 0;
    if (!v) {
        console_write("0");
        return;
    }
    while (v && i < (int)sizeof(buf)-1) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i--) {
        char c[2] = { buf[i], 0 };
        console_write(c);
    }
}

void syscall_init(void)
{
    /* vector 0x80, DPL=3, present, 32-bit interrupt gate */
    idt_set_gate(0x80, (uint32_t)isr80_syscall, 0x08, 0xEE);
}

void syscall_handler(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3)
{
    (void)a2;
    (void)a3;

    switch (num) {
    case SYS_PUTS:
        console_write((const char *)a1);
        break;

    case 0:
        break;

    default:
        console_write("[KERNEL] Unknown syscall: ");
        print_uint(num);
        console_write("\n");
        break;
    }
}
