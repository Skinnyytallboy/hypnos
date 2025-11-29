// // kernel/user/user_program.c

// #include <stdint.h>

// // Simple user-mode text output that writes directly to VGA memory.
// // No kernel calls, no privileged instructions.

// #define VGA_TEXT_BASE 0xB8000
// #define VGA_COLS      80
// #define VGA_ROWS      25

// static uint16_t* const vga = (uint16_t*)VGA_TEXT_BASE;
// static int u_row = 10;   // start a bit lower so it appears under boot text
// static int u_col = 0;

// static void user_putc(char c, uint8_t color)
// {
//     if (c == '\n') {
//         u_row++;
//         u_col = 0;
//     } else {
//         if (u_row >= VGA_ROWS) {
//             // crude scroll: clamp at last line
//             u_row = VGA_ROWS - 1;
//         }
//         vga[u_row * VGA_COLS + u_col] = ((uint16_t)color << 8) | (uint8_t)c;
//         u_col++;
//         if (u_col >= VGA_COLS) {
//             u_col = 0;
//             u_row++;
//         }
//     }
// }

// static void user_puts(const char* s, uint8_t color)
// {
//     while (*s) {
//         user_putc(*s++, color);
//     }
// }

// void user_program_main(void)
// {
//     // 0x0F = white on black
//     user_puts("\n[USER] Hello from Ring 3!\n", 0x0F);
//     user_puts("[USER] I am running with user privileges.\n", 0x0F);
//     user_puts("[USER] If I try privileged instructions, I will fault.\n", 0x0F);

//     // Simple counter loop so you can see it's alive
//     volatile uint32_t counter = 0;
//     while (1) {
//         counter++;
//         // do nothing – just spin in user mode
//     }
// }
