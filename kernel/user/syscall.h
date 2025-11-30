// kernel/user/syscall.h
#pragma once
#include <stdint.h>

/* Must match kernel/syscall.h */
enum {
    SYS_PUTS      = 1,
    SYS_GET_TICKS = 2,
};

/* Write a C string via kernel console */
static inline void sys_puts(const char *s)
{
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(SYS_PUTS), "b"(s)
        : "memory"
    );
}

/* Get current tick count from kernel timer */
static inline uint32_t sys_get_ticks(void)
{
    uint32_t ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)                 /* eax on return */
        : "a"(SYS_GET_TICKS)        /* eax = syscall number */
        : "ebx", "ecx", "edx", "memory"
    );
    return ret;
}
