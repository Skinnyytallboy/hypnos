#pragma once
#include <stdint.h>

typedef struct task {
    uint32_t id;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t* stack;
    struct task* next;
} task_t;

void tasking_init(void);
void task_create(void (*func)(void));
void task_start(void);
void task_yield(void);
