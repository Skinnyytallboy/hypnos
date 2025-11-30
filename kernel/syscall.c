// kernel/syscall.c
#include "syscall.h"
#include "arch/i386/cpu/idt.h"
#include "console.h"
#include "arch/i386/drivers/timer.h"   // timer_get_ticks()

/* Assembly stub for int 0x80 */
extern void isr80_syscall(void);

/* tiny helper just for debugging unknown syscalls */
static void print_uint(uint32_t v)
{
    char buf[16];
    int i = 0;
    if (v == 0) {
        console_write("0");
        return;
    }
    while (v && i < 15) {
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
    /* DPL=3 (0xE -> interrupt gate, bottom 2 bits DPL=3 => 0xEE)
       so user mode (ring 3) can invoke int 0x80. */
    idt_set_gate(0x80, (uint32_t)isr80_syscall, 0x08, 0xEE);
}

uint32_t syscall_handler(uint32_t num,
                         uint32_t a1,
                         uint32_t a2,
                         uint32_t a3)
{
    (void)a2;
    (void)a3;

    switch (num) {
    case SYS_PUTS:
        /* a1 = pointer to user string */
        console_write((const char *)a1);
        return 0;

    case SYS_GET_TICKS:
        /* just pass through the kernel tick counter */
        return timer_get_ticks();

    default:
        console_write("[KERNEL] Unknown syscall: ");
        print_uint(num);
        console_write("\n");
        return (uint32_t)-1;
    }
}
