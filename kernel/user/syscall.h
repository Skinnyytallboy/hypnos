#pragma once
#include <stdint.h>

enum {
    SYS_PUTS = 1,
};

static inline void sys_puts(const char *s)
{
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(SYS_PUTS), "b"(s)
        : "memory"
    );
}
