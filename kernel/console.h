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

/* Internal state (header-only, but synced via HW cursor) */
static uint16_t* const vga_buffer = (uint16_t*)0xB8000;
static uint32_t sleep_timer=1000;
static size_t  console_row   = 0;
static size_t  console_col   = 0;
static uint8_t console_color = (COLOR_LIGHT_GREY | (COLOR_BLACK << 4));

/* --- VGA port I/O helpers (for hardware cursor) --- */
static inline void vga_outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t vga_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void vga_set_hw_cursor(size_t row, size_t col) {
    if (row >= VGA_HEIGHT) row = VGA_HEIGHT - 1;
    if (col >= VGA_WIDTH)  col = VGA_WIDTH - 1;
    uint16_t pos = (uint16_t)(row * VGA_WIDTH + col);

    vga_outb(0x3D4, 0x0F);
    vga_outb(0x3D5, (uint8_t)(pos & 0xFF));
    vga_outb(0x3D4, 0x0E);
    vga_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static inline void vga_get_hw_cursor(size_t* row, size_t* col) {
    uint16_t pos;
    vga_outb(0x3D4, 0x0F);
    pos  = vga_inb(0x3D5);
    vga_outb(0x3D4, 0x0E);
    pos |= ((uint16_t)vga_inb(0x3D5)) << 8;

    size_t r = pos / VGA_WIDTH;
    size_t c = pos % VGA_WIDTH;

    if (r >= VGA_HEIGHT) r = VGA_HEIGHT - 1;
    if (c >= VGA_WIDTH)  c = VGA_WIDTH - 1;

    if (row) *row = r;
    if (col) *col = c;
}

/* Cursor API – synced with HW cursor so all TUs agree */
static inline void console_get_cursor(size_t* row, size_t* col) {
    size_t r, c;
    vga_get_hw_cursor(&r, &c);
    console_row = r;
    console_col = c;
    if (row) *row = r;
    if (col) *col = c;
}

static inline void console_set_cursor(size_t row, size_t col) {
    if (row >= VGA_HEIGHT) row = VGA_HEIGHT - 1;
    if (col >= VGA_WIDTH)  col = VGA_WIDTH - 1;
    console_row = row;
    console_col = col;
    vga_set_hw_cursor(console_row, console_col);
}

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

/*
 * Basic scrolling
 *
 * IMPORTANT: We reserve the LAST ROW (VGA_HEIGHT-1) for the status bar.
 * So we only scroll rows 0..VGA_HEIGHT-2 (content area), and never touch
 * the bottom status-bar row. This prevents the bar text from being
 * scrolled up and mixing with normal shell output.
 */
static inline void console_scroll(void) {
    /* Only scroll when we go past the last CONTENT row (VGA_HEIGHT-2) */
    if (console_row < VGA_HEIGHT - 1)
        return;

    /* move content lines up: rows 1..(HEIGHT-2) -> rows 0..(HEIGHT-3) */
    for (size_t y = 1; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y - 1) * VGA_WIDTH + x] =
                vga_buffer[y * VGA_WIDTH + x];
        }
    }

    /* clear the last CONTENT row (HEIGHT-2) */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 2) * VGA_WIDTH + x] =
            vga_entry(' ', console_color);
    }

    /* after scrolling, keep cursor on the last content row */
    console_row = VGA_HEIGHT - 2;
    if (console_col >= VGA_WIDTH) console_col = 0;
    vga_set_hw_cursor(console_row, console_col);
}

/* Clear screen and reset cursor (status bar will be redrawn later) */
static inline void console_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] =
                vga_entry(' ', console_color);
        }
    }
    console_row = 0;
    console_col = 0;
    vga_set_hw_cursor(console_row, console_col);
}

/* Print a single character */
static inline void console_putc(char c) {
    if (c == '\n') {
        console_col = 0;
        console_row++;
        console_scroll();
        vga_set_hw_cursor(console_row, console_col);
        return;
    }

    if (c == '\r') {
        console_col = 0;
        vga_set_hw_cursor(console_row, console_col);
        return;
    }

    if (c == '\b') {
        if (console_col > 0) {
            console_col--;
            console_put_at(' ', console_col, console_row);
        }
        vga_set_hw_cursor(console_row, console_col);
        return;
    }

    if (console_col >= VGA_WIDTH) {
        console_col = 0;
        console_row++;
        console_scroll();
    }

    /* Make sure we never print normal text into the status bar row */
    if (console_row >= VGA_HEIGHT - 1) {
        console_scroll();
    }

    console_put_at(c, console_col, console_row);
    console_col++;
    vga_set_hw_cursor(console_row, console_col);
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
