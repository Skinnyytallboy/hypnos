#pragma once
#include <stdint.h>
#include <stddef.h>

/* VGA text mode: 80x25, memory at 0xB8000 */

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

enum vga_color {
    COLOR_BLACK         = 0,
    COLOR_BLUE          = 1,
    COLOR_GREEN         = 2,
    COLOR_CYAN          = 3,
    COLOR_RED           = 4,
    COLOR_MAGENTA       = 5,
    COLOR_BROWN         = 6,
    COLOR_LIGHT_GREY    = 7,
    COLOR_DARK_GREY     = 8,
    COLOR_LIGHT_BLUE    = 9,
    COLOR_LIGHT_GREEN   = 10,
    COLOR_LIGHT_CYAN    = 11,
    COLOR_LIGHT_RED     = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN   = 14,
    COLOR_WHITE         = 15,
};

/* Internal state */
static uint16_t* const vga_buffer = (uint16_t*)0xB8000;
static size_t  console_row   = 0;
static size_t  console_col   = 0;
static uint8_t console_color = (COLOR_LIGHT_GREY | (COLOR_BLACK << 4));

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

static inline uint16_t vga_entry(unsigned char ch, uint8_t color) {
    return (uint16_t)ch | ((uint16_t)color << 8);
}

static inline void console_set_color(uint8_t fg, uint8_t bg) {
    console_color = vga_entry_color(fg, bg);
}

static inline void console_put_at(char c, size_t x, size_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) return;
    const size_t idx = y * VGA_WIDTH + x;
    vga_buffer[idx] = vga_entry(c, console_color);
}

/* Basic scrolling */
static inline void console_scroll(void) {
    if (console_row < VGA_HEIGHT)
        return;
    /* move each line up */
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y - 1) * VGA_WIDTH + x] =
                vga_buffer[y * VGA_WIDTH + x];
        }
    }
    /* clear last line */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            vga_entry(' ', console_color);
    }
    console_row = VGA_HEIGHT - 1;
}

/* Clear screen and reset cursor */
static inline void console_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] =
                vga_entry(' ', console_color);
        }
    }
    console_row = 0;
    console_col = 0;
}

/* Print a single character */
static inline void console_putc(char c) {
    if (c == '\n') {
        console_col = 0;
        console_row++;
        console_scroll();
        return;
    }

    if (c == '\r') {
        console_col = 0;
        return;
    }

    if (c == '\b') {
        if (console_col > 0) {
            console_col--;
            console_put_at(' ', console_col, console_row);
        }
        return;
    }

    if (console_col >= VGA_WIDTH) {
        console_col = 0;
        console_row++;
        console_scroll();
    }

    console_put_at(c, console_col, console_row);
    console_col++;
}

/* Print a string */
static inline void console_write(const char* s) {
    if (!s) return;
    while (*s) {
        console_putc(*s++);
    }
}

/* Optional: set a "theme" */
static inline void console_set_theme_default(void) {
    console_set_color(COLOR_LIGHT_GREY, COLOR_BLACK);
}

static inline void console_set_theme_banner(void) {
    console_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
}

static inline void console_set_theme_error(void) {
    console_set_color(COLOR_LIGHT_RED, COLOR_BLACK);
}

static inline void console_set_theme_ok(void) {
    console_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
}
