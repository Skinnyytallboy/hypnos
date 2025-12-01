#include "sched/task.h"
#include "arch/i386/mm/kmalloc.h"
#include "console.h"
#include "log.h"

extern void start_task(uint32_t esp, uint32_t eip);
extern void switch_task(cpu_state_t *old, cpu_state_t *new);

static task_t *current   = 0;
static task_t *task_head = 0;
static int     next_id   = 1;

void task_init(void)
{
    current   = 0;
    task_head = 0;
    next_id   = 1;
    log_event("[SCHED] task subsystem initialized.");
}

static void task_append(task_t *t)
{
    if (!task_head) {
        task_head = t;
        t->next   = t;
        return;
    }

    task_t *p = task_head;
    while (p->next != task_head)
        p = p->next;

    p->next = t;
    t->next = task_head;
}

task_t *task_create(void (*entry)(void), const char *name)
{
    const uint32_t STACK_SIZE = 4096;

    task_t *t = kmalloc(sizeof(task_t));
    if (!t) {
        console_write("task_create: kmalloc task failed\n");
        log_event("[SCHED] task_create: kmalloc task FAILED.");
        return 0;
    }

    uint8_t *stack = kmalloc(STACK_SIZE);
    if (!stack) {
        console_write("task_create: kmalloc stack failed\n");
        log_event("[SCHED] task_create: kmalloc stack FAILED.");
        return 0;
    }

    uint32_t top = (uint32_t)stack + STACK_SIZE;

    // Clear context
    for (int i = 0; i < (int)(sizeof(cpu_state_t) / 4); i++)
        ((uint32_t*)&t->regs)[i] = 0;

    t->regs.eip    = (uint32_t)entry;
    t->regs.esp    = top;
    t->regs.ebp    = top;
    t->regs.eflags = 0x202;

    t->regs.cs = 0x08;
    t->regs.ds = 0x10;
    t->regs.es = 0x10;
    t->regs.fs = 0x10;
    t->regs.gs = 0x10;
    t->regs.ss = 0x10;

    t->stack_base = stack;
    t->stack_size = STACK_SIZE;

    t->name = name;
    t->id   = next_id++;

    task_append(t);

    // console_write("Task created: ");
    // console_write(name);
    // console_write("\n");

    log_event("[SCHED] task created.");
    // If you want to also log the name explicitly:
    log_event(name);

    return t;
}

void task_yield(void)
{
    if (!current || current->next == current)
        return;

    task_t *old = current;
    task_t *new = current->next;
    current = new;

    log_event("[SCHED] task_yield() - switching task.");

    __asm__ volatile("cli");
    switch_task(&old->regs, &new->regs);
    __asm__ volatile("sti");
}

void scheduler_start(void)
{
    if (!task_head) {
        console_write("scheduler_start: no tasks!\n");
        log_event("[SCHED] scheduler_start: no tasks (HALT).");
        for (;;) __asm__("hlt");
    }

    current = task_head;

    // console_write("Starting scheduler with task: ");
    // console_write(current->name);
    // console_write("\n");

    log_event("[SCHED] scheduler_start: starting with first task.");
    log_event(current->name);

    start_task(current->regs.esp, current->regs.eip);

    console_write("scheduler_start: returned unexpectedly!\n");
    log_event("[SCHED] scheduler_start returned unexpectedly (BUG).");
    for (;;) __asm__("hlt");
}
