// kernel/sched/task.c
#include "sched/task.h"
#include "arch/i386/mm/kmalloc.h"
#include "console.h"

extern void start_task(uint32_t new_esp, uint32_t new_ebp, uint32_t new_eip);
extern void switch_task(uint32_t *old_esp, uint32_t *old_ebp, uint32_t *old_eip,
                        uint32_t  new_esp, uint32_t  new_ebp, uint32_t  new_eip);

#define MAX_TASKS        16
#define TASK_STACK_SIZE  4096

static task_t tasks[MAX_TASKS];
static task_t *current    = 0;
static int     current_id = -1;

/* set by timer IRQ, consumed by tasks via task_yield_if_needed() */
static volatile int need_resched = 0;

void scheduler_tick(void)
{
    need_resched = 1;
}

task_t *task_current(void)
{
    return current;
}

void tasking_init(void)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].active     = 0;
        tasks[i].stack_base = 0;
        tasks[i].esp        = 0;
        tasks[i].ebp        = 0;
        tasks[i].eip        = 0;
        tasks[i].id         = i;
    }
    current    = 0;
    current_id = -1;
}

/* round-robin: find next active task after current_id */
static int pick_next_id(void)
{
    if (current_id < 0) {
        /* pick first active task */
        for (int i = 0; i < MAX_TASKS; i++)
            if (tasks[i].active)
                return i;
        return -1;
    }

    int start = current_id;
    int i     = (start + 1) % MAX_TASKS;

    while (i != start) {
        if (tasks[i].active)
            return i;
        i = (i + 1) % MAX_TASKS;
    }

    /* maybe only one active task */
    if (tasks[start].active)
        return start;

    return -1;
}

int task_create(void (*entry)(void))
{
    int idx = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!tasks[i].active) {
            idx = i;
            break;
        }
    }
    if (idx < 0)
        return -1;

    uint8_t *stack = (uint8_t *)kmalloc(TASK_STACK_SIZE);
    if (!stack)
        return -1;

    uint32_t top = (uint32_t)stack + TASK_STACK_SIZE;

    tasks[idx].stack_base = stack;
    tasks[idx].esp        = top;
    tasks[idx].ebp        = top;
    tasks[idx].eip        = (uint32_t)entry;
    tasks[idx].active     = 1;

    if (!current) {
        current    = &tasks[idx];
        current_id = idx;
    }

    return idx;
}

void task_start_first(void)
{
    int id = pick_next_id();
    if (id < 0) {
        console_write("task_start_first: no runnable tasks!\n");
        for (;;) __asm__ volatile ("hlt");
    }

    current    = &tasks[id];
    current_id = id;

    start_task(current->esp, current->ebp, current->eip);
    /* never returns */
}

void task_yield(void)
{
    int next_id = pick_next_id();
    if (next_id < 0 || next_id == current_id)
        return;

    task_t *old = current;
    task_t *new = &tasks[next_id];

    current    = new;
    current_id = next_id;

    switch_task(&old->esp, &old->ebp, &old->eip,
                new->esp,  new->ebp,  new->eip);
    /* returns in context of 'new' later */
}

void task_yield_if_needed(void)
{
    if (need_resched) {
        need_resched = 0;
        task_yield();
    }
}
