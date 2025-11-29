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

// void thread1(void)
// {
//     while (1) {
//         console_write("[T1]");
//         task_yield();
//     }
// }

// void thread2(void)
// {
//     while (1) {
//         console_write("[T2]");
//         task_yield();
//     }
// }

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

static void ok(const char* msg) {
    console_set_theme_ok();
    console_write("[OK] ");
    console_set_theme_default();
    console_write(msg);
    console_write("\n");
}

static void banner(const char* msg) {
    console_set_theme_banner();
    console_write(msg);
    console_write("\n");
    console_set_theme_default();
}
static void loading_animation(const char* msg)
{
    console_write(msg);
    for (int i = 0; i < 3; i++) {
        for (volatile int j = 0; j < 2000000; j++) {
            __asm__ volatile ("nop");
        }
        console_write(".");
    }
    console_write("\n");
}


void kernel_main(void)
{
    console_set_theme_default();
    console_clear();

    print_ascii_banner();
    loading_animation("Booting Hypnos");

    banner("=== Hypnos kernel booted ===");

    gdt_init();
    ok("GDT initialized.");

    idt_init();
    ok("IDT initialized.");

    paging_init();
    paging_enable();
    ok("Paging initialized and enabled.");

    phys_init();
    ok("Physical memory manager initialized.");

    kmalloc_init();
    ok("Kernel heap initialized.");

    {
        char* buf = kmalloc(32);
        if (buf) {
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

    crypto_set_key("hypnos-default-key");
    ok("Filesystem encryption key installed.");

    fs_init();
    ok("Filesystem initialized.");

    irq_install();
    ok("PIC remapped, IRQs installed.");

    timer_install();
    ok("Timer initialized (100 Hz).");

    keyboard_install();
    ok("Keyboard driver installed.");

    console_write("Enabling interrupts...\n");
    __asm__ volatile ("sti");

    banner("Starting Hypnos Shell...");
    shell_init();
    shell_run();

    for (;;) __asm__ volatile ("hlt");
}