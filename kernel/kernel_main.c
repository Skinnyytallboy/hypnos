#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "arch/i386/cpu/gdt.h"
#include "arch/i386/cpu/idt.h"
#include "arch/i386/cpu/irq.h"
#include "arch/i386/drivers/timer.h"
#include "arch/i386/drivers/keyboard.h"
#include "arch/i386/mm/paging.h"
#include "arch/i386/mm/physmem.h"
#include "arch/i386/mm/kmalloc.h"
#include "shell/shell.h"
// #include "task.h"
#include "fs/fs.h"
#include "fs/crypto.h"
#include "log.h"
#include "security.h"
#include "syscall.h"

static void kprint_u32(uint32_t v)
{
    char buf[16];
    int i = 0;

    if (v == 0)
    {
        console_write("0");
        return;
    }

    while (v > 0 && i < (int)sizeof(buf))
    {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i > 0)
        console_putc(buf[--i]);
}

extern volatile uint32_t timer_ticks;

static void sleep_ticks(uint32_t ticks)
{
    if (ticks == 0)
        return;

    uint32_t start = timer_ticks;

    if (start == 0)
    {
        /* Timer not running yet: approximate delay using a busy loop.
         * The constant here is arbitrary; tune it with sleep_timer.
         */
        for (volatile uint32_t i = 0; i < ticks * 100000; ++i)
        {
            __asm__ volatile("nop");
        }
        return;
    }
    while ((uint32_t)(timer_ticks - start) < ticks)
    {
        __asm__ volatile("hlt");
    }
}

static void print_ascii_banner(void)
{
    console_set_theme_banner();
    console_write(" _   _                          \n");
    console_write("| | | | _   _ _ __   ___  ___   \n");
    console_write("| |_| || | | | '_ \\ / __|/ _ \\  \n");
    console_write("|  _  || |_| | | | |\\__ \\  __/  \n");
    console_write("|_| |_| \\__,_|_| |_||___/\\___|  \n");
    console_write("           H Y P N O S          \n\n");
    console_set_theme_default();
}

static void ok(const char *msg)
{
    console_set_theme_ok();
    console_write("[OK] ");
    console_set_theme_default();
    console_write(msg);
    console_write("\n");
}

static void banner(const char *msg)
{
    console_set_theme_banner();
    console_write(msg);
    console_write("\n");
    console_set_theme_default();
}
static void loading_animation(const char *msg)
{
    console_write(msg);
    for (int i = 0; i < 3; i++)
    {
        for (volatile int j = 0; j < 2000000; j++)
        {
            __asm__ volatile("nop");
        }
        console_write(".");
    }
    console_write("\n");
}

void kernel_main(void)
{
    console_set_theme_default();
    console_clear();

    gdt_init();
    ok("GDT initialized.");
    sleep_ticks(sleep_timer);

    idt_init();
    ok("IDT initialized.");
    sleep_ticks(sleep_timer);

    paging_init();
    paging_enable();
    ok("Paging initialized and enabled.");
    sleep_ticks(sleep_timer);

    phys_init();
    ok("Physical memory manager initialized.");
    sleep_ticks(sleep_timer);

    kmalloc_init();
    ok("Kernel heap initialized.");
    sleep_ticks(sleep_timer);

    {
        char *buf = kmalloc(32);
        if (buf)
        {
            buf[0] = 'O';
            buf[1] = 'K';
            buf[2] = 0;
            console_write("Heap test: ");
            console_write(buf);
            console_write("\n");
        }
    }

    sec_init();
    console_write("Security subsystem initialized. Current user: ");
    console_write(sec_get_current_username());
    console_write("\n");
    sleep_ticks(sleep_timer);

    crypto_set_key("hypnos-default-key");
    ok("Filesystem encryption key installed.");
    sleep_ticks(sleep_timer);

    fs_init();
    ok("Filesystem initialized.");
    sleep_ticks(sleep_timer);

    irq_install();
    ok("PIC remapped, IRQs installed.");
    sleep_ticks(sleep_timer);

    // Added the test: now the flags should be 142, if 0 then IRQs not installed
    // Also the base_lo and base_hi should be non-zero
    {
        extern struct idt_entry *idt_get_array();
        struct idt_entry *idt_arr = idt_get_array();

        console_write("IRQ0 flags = ");
        kprint_u32(idt_arr[32].flags);
        console_write(" base_lo = ");
        kprint_u32(idt_arr[32].base_lo);
        console_write(" base_hi = ");
        kprint_u32(idt_arr[32].base_hi);
        console_write("\n");
    }

    syscall_init();
    ok("Syscalls (INT 0x80) initialized.");
    sleep_ticks(sleep_timer);

    timer_install();
    ok("Timer initialized (100 Hz).");
    sleep_ticks(sleep_timer);

    keyboard_install();
    ok("Keyboard driver installed.");
    sleep_ticks(sleep_timer);

    console_write("Enabling interrupts...\n");
    sleep_ticks(sleep_timer);
    __asm__ volatile("sti");

    console_clear();
    print_ascii_banner();
    banner("=== Hypnos kernel booted ===");
    banner("Starting Hypnos Shell...");
    shell_init();
    shell_run();

    // extern void switch_to_user_mode(void);
    // console_write("\nLaunching first user program...\n");
    // switch_to_user_mode();   // jumps to Ring 3, never returns

    for (;;)
    {
        __asm__ volatile("hlt");
    }
}
