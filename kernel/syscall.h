#pragma once
#include <stdint.h>

enum {
    SYS_PUTS      = 1,
    SYS_GET_TICKS = 2,
    // add more later
};

void syscall_init(void);

__attribute__((cdecl))
__attribute__((noinline))
uint32_t syscall_handler(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3);
