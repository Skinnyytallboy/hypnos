#include <stdint.h>
#include "arch/i386/cpu/irq.h"
#include "console.h"

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

//static volatile uint32_t timer_ticks = 0;
volatile uint32_t timer_ticks = 0;

static void timer_callback(void) {
    timer_ticks++;
    // if (timer_ticks % 100 == 0)   // roughly once per second at 100 Hz
    //     console_write("Tick...\n");
    // if (timer_ticks % 100 == 0) console_write(".");
    // extern void task_switch(void);
    // task_switch();
}

void timer_install(void)
{
    /* PIT frequency: 1193180 Hz / divisor = desired frequency */
    uint32_t freq = 100;           // 100 Hz
    uint32_t divisor = 1193180 / freq;

    outb(0x43, 0x36);              // command: channel 0, lobyte/hibyte, mode 3, binary
    outb(0x40, divisor & 0xFF);    // low byte
    outb(0x40, (divisor >> 8) & 0xFF); // high byte

    irq_register_handler(0, timer_callback);
}
