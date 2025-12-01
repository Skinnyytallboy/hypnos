#include "syscall.h"
#include "console.h"
#include "arch/i386/cpu/idt.h"
#include "arch/i386/drivers/timer.h"

static void kprint_u32(uint32_t v)
{
    char buf[16];
    int  i = 0;

    if (v == 0) {
        console_write("0");
        return;
    }

    while (v > 0 && i < (int)sizeof(buf)) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i > 0)
        console_putc(buf[--i]);
}

void syscall_init(void)
{
    console_write("Installing syscall at: ");
    extern void isr80_syscall();
    kprint_u32((uint32_t)isr80_syscall);
    console_write("\n");

    // 0x8E = present, ring0, 32-bit interrupt gate
    // 0xEE = present, ring3 (DPL=3), 32-bit interrupt gate
    idt_set_gate(0x80, (uint32_t)isr80_syscall, 0x08, 0xEE);

    struct idt_entry* idt_arr = idt_get_array();

    console_write("IDT[80] base_hi: ");
    kprint_u32(idt_arr[0x80].base_hi);
    console_write(" base_lo: ");
    kprint_u32(idt_arr[0x80].base_lo);
    console_write(" flags: ");
    kprint_u32(idt_arr[0x80].flags);
    console_write("\n");
}


__attribute__((cdecl))
__attribute__((noinline))
uint32_t syscall_handler(uint32_t num,
                         uint32_t a1,
                         uint32_t a2,
                         uint32_t a3)
{
    switch (num) {
    case SYS_PUTS:
        console_write((const char*)a1);
        return 0;

    case SYS_GET_TICKS:
        return timer_get_ticks();

    default:
        console_write("[KERNEL] Unknown syscall: ");
        kprint_u32(num);
        console_write("\n");
        return (uint32_t)-1;
    }
}
