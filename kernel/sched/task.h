#pragma once
#include <stdint.h>

typedef struct cpu_state {
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
} cpu_state_t;

typedef struct task {
    cpu_state_t regs;
    struct task *next;
    const char *name;
    int id;
} task_t;

void task_init(void);
task_t *task_create(void (*entry)(void), const char *name);
void task_yield(void);
void scheduler_start(void);
