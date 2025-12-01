#pragma once
#include <stdint.h>

enum {
    SYS_PUTS      = 1,
    SYS_GET_TICKS = 2,
};

static inline uint32_t sys_call3(uint32_t num,
                                 uint32_t a1,
                                 uint32_t a2,
                                 uint32_t a3)
{
    uint32_t ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(num), "b"(a1), "c"(a2), "d"(a3)
                     : "memory");
    return ret;
}

static inline void sys_puts(const char *s)
{
    (void)sys_call3(SYS_PUTS, (uint32_t)s, 0, 0);
}

static inline uint32_t sys_get_ticks(void)
{
    return sys_call3(SYS_GET_TICKS, 0, 0, 0);
}
