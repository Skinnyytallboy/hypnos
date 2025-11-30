// kernel/syscall.h
#pragma once
#include <stdint.h>

/* These numbers must match user/syscall.h */
enum {
    SYS_PUTS      = 1,   // write string at (const char *)a1
    SYS_GET_TICKS = 2,   // returns uint32_t in eax
};

void syscall_init(void);

/* Main dispatcher, called from isr80_syscall stub.
 * Returns value that will go back to user in EAX.
 */
uint32_t syscall_handler(uint32_t num,
                         uint32_t a1,
                         uint32_t a2,
                         uint32_t a3);
