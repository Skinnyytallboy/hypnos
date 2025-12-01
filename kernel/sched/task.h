#pragma once
#include <stdint.h>


// (GP registers + segment registers + EFLAGS + EIP)

typedef struct cpu_state {
    uint32_t eax;     
    uint32_t ebx;      
    uint32_t ecx;      
    uint32_t edx;      
    uint32_t esi;      
    uint32_t edi;      
    uint32_t ebp;      
    uint32_t esp;      
    uint32_t eip;      
    uint32_t eflags;
    uint32_t cs;       
    uint32_t ds;       
    uint32_t es;       
    uint32_t fs;       
    uint32_t gs;       
    uint32_t ss;       
} cpu_state_t;

typedef struct task {
    cpu_state_t regs;

    uint8_t *stack_base;
    uint32_t stack_size;

    struct task *next;
    const char *name;
    int id;
} task_t;

void task_init(void);
task_t *task_create(void (*entry)(void), const char *name);
void task_yield(void);
void scheduler_start(void);
