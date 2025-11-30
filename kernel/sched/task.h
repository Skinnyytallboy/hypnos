// kernel/sched/task.h
#pragma once
#include <stdint.h>

typedef struct task {
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint8_t *stack_base;   // kmalloc()’d stack base
    int      id;
    int      active;
} task_t;

/* Initialise tasking subsystem (must be called once, before creating tasks) */
void tasking_init(void);

/* Create a new kernel task that starts at 'entry' (never returns). 
 * Returns task id >= 0 on success, -1 on failure.
 */
int task_create(void (*entry)(void));

/* Explicit yield from the current task to the next runnable task */
void task_yield(void);

/* Called frequently from tasks; yields only if timer requested reschedule */
void task_yield_if_needed(void);

/* Called from timer IRQ: just sets a flag so tasks know to yield soon */
void scheduler_tick(void);

/* Start the first task; does not return.
 * Must be called once after creating initial tasks.
 */
void task_start_first(void);

/* Optional: get pointer to current task (for debugging / future use) */
task_t *task_current(void);
