#include <stdint.h>
#include <stddef.h>

#define VGA_TEXT ((uint16_t*)0xB8000)
#define VGA_COLS 80
#define VGA_ROWS 25
static uint8_t vga_color = 0x0F; // white on black
static size_t cur_row = 0, cur_col = 0;

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void cls(void) {
    for (size_t i = 0; i < VGA_ROWS * VGA_COLS; ++i) {
        VGA_TEXT[i] = vga_entry(' ', vga_color);
    }
    cur_row = cur_col = 0;
}

static void putc(char c) {
    if (c == '\n') { cur_col = 0; if (++cur_row >= VGA_ROWS) cur_row = 0; return; }
    size_t idx = cur_row * VGA_COLS + cur_col;
    VGA_TEXT[idx] = vga_entry(c, vga_color);
    if (++cur_col >= VGA_COLS) { cur_col = 0; if (++cur_row >= VGA_ROWS) cur_row = 0; }
}

static void puts(const char* s) { while (*s) putc(*s++); }

void kernel_main(void) {
    cls();
    puts("Hypnos kernel booted! Hello from 32-bit land.\n");
    puts("Clean screen, no firmware noise. :)\n");
    for (;;) __asm__ volatile ("hlt");
}
