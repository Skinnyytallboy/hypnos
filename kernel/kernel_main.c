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
#include "sched/task.h"
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

extern volatile uint32_t timer_ticks;   



static void sleep_ticks(uint32_t ticks)
{
    if (ticks == 0)
        return;

    uint32_t start = timer_get_ticks();

    if (start == 0) {
        /* Timer not running yet: approximate delay using a busy loop. */
        for (volatile uint32_t i = 0; i < ticks * 100000; ++i) {
            __asm__ volatile("nop");
        }
        return;
    }

    while ((uint32_t)(timer_get_ticks() - start) < ticks) {
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

extern void shell_init(void);
extern void shell_run(void);

static void shell_thread(void)
{
    shell_init();
    shell_run();   // never returns
}

static void demo_task(void)
{
    size_t y = 10;
    size_t x = 0;
    while (1) {
        console_set_color(COLOR_GREEN, COLOR_BLACK);
        char c = 'M';
        console_put_at(c, x, y);
        x++;
        if (x >= VGA_WIDTH) {
            x = 0;
            for (size_t i = 0; i < VGA_WIDTH; i++)
                console_put_at(' ', i, y);
        }
        console_set_theme_default();
        task_yield();
    }
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

    timer_install();
    ok("Timer initialized (100 Hz).");
        sleep_ticks(sleep_timer);

    keyboard_install();
    ok("Keyboard driver installed.");
        sleep_ticks(sleep_timer);

    console_write("Enabling interrupts...\n");
    sleep_ticks(sleep_timer);
    __asm__ volatile ("sti");

    task_init();

    console_clear();
    print_ascii_banner();
    banner("=== Hypnos kernel booted ===");
    banner("Starting multitasking demo...");

    task_create(shell_thread, "shell");

    task_create(demo_task, "demo");

    scheduler_start();
}