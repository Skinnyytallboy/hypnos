#include <stdint.h>
#include "arch/i386/cpu/irq.h"

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* global tick counter */
volatile uint32_t timer_ticks = 0;

/* forward decl – implemented in shell.c */
void shell_tick(void);

uint32_t timer_get_ticks(void)
{
    return timer_ticks;
}

uint32_t timer_get_seconds(void)
{
    return timer_ticks / 100;
}

static void timer_callback(void)
{
    timer_ticks++;

    /* Notify shell once per tick; shell_tick() will throttle itself */
    shell_tick();
}

void timer_install(void)
{
    uint32_t freq    = 100;
    uint32_t divisor = 1193180 / freq;

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    irq_register_handler(0, timer_callback);
}
