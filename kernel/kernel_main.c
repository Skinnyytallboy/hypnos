#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "timer.h"
#include "keyboard.h"
#include "paging.h"
#include "kmalloc.h"
#include "shell.h"
#include "physmem.h"
// #include "task.h"
#include "fs.h"

#define VGA_MEMORY   0xB8000
#define VGA_COLS     80
#define VGA_ROWS     25

static uint16_t* const VGA = (uint16_t*)VGA_MEMORY;
static uint8_t text_color = 0x0F;

static size_t cursor_row = 0;
static size_t cursor_col = 0;

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void console_scroll_if_needed(void) {
    if (cursor_row < VGA_ROWS) return;
    for (size_t r = 1; r < VGA_ROWS; ++r) {
        for (size_t c = 0; c < VGA_COLS; ++c) VGA[(r - 1) * VGA_COLS + c] = VGA[r * VGA_COLS + c];
    }
    for (size_t c = 0; c < VGA_COLS; ++c) VGA[(VGA_ROWS - 1) * VGA_COLS + c] = vga_entry(' ', text_color);

    cursor_row = VGA_ROWS - 1;
}

static void console_putc(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        console_scroll_if_needed();
        return;
    }

    if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            VGA[cursor_row * VGA_COLS + cursor_col] = vga_entry(' ', text_color);
        }
        return;
    }

    VGA[cursor_row * VGA_COLS + cursor_col] = vga_entry(c, text_color);
    cursor_col++;

    if (cursor_col >= VGA_COLS) {
        cursor_col = 0;
        cursor_row++;
        console_scroll_if_needed();
    }
}

void console_clear(void) {
    for (size_t r = 0; r < VGA_ROWS; ++r) {
        for (size_t c = 0; c < VGA_COLS; ++c) {
            VGA[r * VGA_COLS + c] = vga_entry(' ', text_color);
        }
    }
    cursor_row = 0;
    cursor_col = 0;
}

void console_write(const char* s) {
    for (size_t i = 0; s[i] != '\0'; ++i) {
        console_putc(s[i]);
    }
}

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

void kernel_main(void) {
    console_clear();
    console_write("Hypnos kernel booted!\n");

    gdt_init();
    console_write("GDT initialized.\n");

    idt_init();
    console_write("IDT initialized.\n");

    paging_init();
    paging_enable();
    console_write("Paging initialized and enabled.\n");

    phys_init();
    console_write("Testing phys_alloc_frame...\n");
    uint32_t f1 = phys_alloc_frame();
    uint32_t f2 = phys_alloc_frame();

    if (f1 && f2) {
        console_write("Allocated frames at: 0x");
        uint32_t vals[2] = { f1, f2 };
        for (int k = 0; k < 2; k++) {
            uint32_t v = vals[k];
            char buf[9];
            for (int i = 0; i < 8; i++) {
                uint8_t nibble = (v >> ((7 - i) * 4)) & 0xF;
                buf[i] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
            }
            buf[8] = 0;
            console_write(buf);
            if (k == 0) console_write(", 0x");
        }
        console_write("\n");
    }

    kmalloc_init();
    console_write("Testing kmalloc...\n");

    void* a = kmalloc(64);
    void* b = kmalloc(128);
    if (a && b)
        console_write("kmalloc working.\n");

    char* buf = kmalloc(256);
    if (buf) {
        buf[0] = 'O';
        buf[1] = 'K';
        buf[2] = 0;
        console_write("Heap test: ");
        console_write(buf);
        console_write("\n");
    }

    fs_init();

    irq_install();
    console_write("PIC remapped, IRQs installed.\n");

    timer_install();
    console_write("Timer initialized (100 Hz).\n");

    keyboard_install();
    console_write("Keyboard driver installed.\n");

    console_write("Enabling interrupts...\n");
    __asm__ volatile ("sti");

    console_write("Starting Hypnos shell...\n");
    shell_init();
    shell_run();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}