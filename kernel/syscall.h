#pragma once
#include <stdint.h>

enum {
    SYS_PUTS = 1,   /* write zero-terminated string at (const char*)ebx */
    /* more later */
};

void syscall_init(void);
void syscall_handler(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3);
