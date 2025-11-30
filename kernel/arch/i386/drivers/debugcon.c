#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

#define DEBUGCON_PORT 0xE9

void debugcon_putc(char c)
{
    outb(DEBUGCON_PORT, (uint8_t)c);
}

void debugcon_write(const char *s)
{
    while (*s) {
        // Optional CRLF normalization
        if (*s == '\n')
            debugcon_putc('\r');
        debugcon_putc(*s++);
    }
}
