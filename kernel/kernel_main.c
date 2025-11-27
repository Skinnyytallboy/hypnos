#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "gdt.h"
#include "idt.h"

#define VGA_MEMORY   0xB8000
#define VGA_COLS     80
#define VGA_ROWS     25

static uint16_t* const VGA = (uint16_t*)VGA_MEMORY;
static uint8_t text_color = 0x0F;   // white on black

static size_t cursor_row = 0;
static size_t cursor_col = 0;

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void console_scroll_if_needed(void) {
    if (cursor_row < VGA_ROWS)
        return;

    // scroll up
    for (size_t r = 1; r < VGA_ROWS; ++r) {
        for (size_t c = 0; c < VGA_COLS; ++c) {
            VGA[(r - 1) * VGA_COLS + c] = VGA[r * VGA_COLS + c];
        }
    }

    // clear last row
    for (size_t c = 0; c < VGA_COLS; ++c) {
        VGA[(VGA_ROWS - 1) * VGA_COLS + c] = vga_entry(' ', text_color);
    }

    cursor_row = VGA_ROWS - 1;
}

static void console_putc(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        console_scroll_if_needed();
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

/* Exported console API (used by other files) */
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

void kernel_main(void) {
    console_clear();
    console_write("Hypnos kernel booted!\n");
    gdt_init();
    console_write("GDT initialized.\n");
    idt_init();
    console_write("IDT initialized.\n");
    console_write("Triggering interrupt 0 (Division By Zero)...\n");
    __asm__ volatile ("int $0");   // this MUST enter your isr0 handler
    console_write("If you see this line, ISR0 didn't run.\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}



// #include <stdint.h>
// #include <stddef.h>
// #include "gdt.h"
// #include "idt.h"


// #define VGA_MEMORY   0xB8000
// #define VGA_COLS     80
// #define VGA_ROWS     25

// static uint16_t* const VGA = (uint16_t*)VGA_MEMORY;
// static uint8_t text_color = 0x0F;   // white on black

// static size_t cursor_row = 0;
// static size_t cursor_col = 0;

// static inline uint16_t vga_entry(char c, uint8_t color) {
//     return (uint16_t)c | ((uint16_t)color << 8);
// }

// static void console_clear(void) {
//     for (size_t r = 0; r < VGA_ROWS; ++r) {
//         for (size_t c = 0; c < VGA_COLS; ++c) {
//             VGA[r * VGA_COLS + c] = vga_entry(' ', text_color);
//         }
//     }
//     cursor_row = 0;
//     cursor_col = 0;
// }

// static void console_scroll_if_needed(void) {
//     if (cursor_row < VGA_ROWS)
//         return;

//     // scroll everything up by one row
//     for (size_t r = 1; r < VGA_ROWS; ++r) {
//         for (size_t c = 0; c < VGA_COLS; ++c) {
//             VGA[(r - 1) * VGA_COLS + c] = VGA[r * VGA_COLS + c];
//         }
//     }

//     // clear last row
//     for (size_t c = 0; c < VGA_COLS; ++c) {
//         VGA[(VGA_ROWS - 1) * VGA_COLS + c] = vga_entry(' ', text_color);
//     }

//     cursor_row = VGA_ROWS - 1;
// }

// static void console_putc(char c) {
//     if (c == '\n') {
//         cursor_col = 0;
//         cursor_row++;
//         console_scroll_if_needed();
//         return;
//     }

//     VGA[cursor_row * VGA_COLS + cursor_col] = vga_entry(c, text_color);
//     cursor_col++;

//     if (cursor_col >= VGA_COLS) {
//         cursor_col = 0;
//         cursor_row++;
//         console_scroll_if_needed();
//     }
// }

// static void console_write(const char* s) {
//     for (size_t i = 0; s[i] != '\0'; ++i) {
//         console_putc(s[i]);
//     }
// }

// void kernel_main(void) {
//     console_clear();
//     console_write("Hypnos booting...\n");

//     gdt_init();
//     console_write("GDT loaded.\n");

//     idt_init();
//     console_write("IDT loaded.\n");

//     console_write("If no crash, interrupts are now active.\n");

//     for (;;) __asm__("hlt");
// }


// // void kernel_main(void) {
// //     console_clear();
// //     console_write("Hypnos kernel booted!\n");
// //     console_write("Welcome, Jalal.\n");
// //     console_write("This is your tiny text console.\n");
// //     console_write("Next: GDT/IDT, interrupts, paging...\n");

// //     for (;;) {
// //         __asm__ volatile ("hlt");
// //     }
// // }
