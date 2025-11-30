#include "sched/task.h"
#include "arch/i386/mm/kmalloc.h"
#include "console.h"

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

    task_t *t = (task_t*)kmalloc(sizeof(task_t));
    if (!t) {
        console_write("task_create: kmalloc task failed\n");
        return 0;
    }

    uint8_t *stack = (uint8_t*)kmalloc(STACK_SIZE);
    if (!stack) {
        console_write("task_create: kmalloc stack failed\n");
        return 0;
    }

    uint32_t top = (uint32_t)stack + STACK_SIZE;

    t->regs.esp = top;
    t->regs.ebp = top;
    t->regs.eip = (uint32_t)entry;

    t->name = name;
    t->id   = next_id++;

    task_append(t);

    console_write("Task created: ");
    console_write(name);
    console_write("\n");

    return t;
}

void task_yield(void)
{
    if (!current || current->next == current) {
        return;
    }

    task_t *old = current;
    task_t *new = current->next;
    current = new;

    __asm__ volatile("cli");
    switch_task(&old->regs, &new->regs);
    console_write("Returned to task: ");
    console_write(current->name);
    __asm__ volatile("sti");
}

void scheduler_start(void)
{
    if (!task_head) {
        console_write("scheduler_start: no tasks!\n");
        for (;;) __asm__ volatile("hlt");
    }

    current = task_head;

    console_write("Starting scheduler with task: ");
    console_write(current->name);
    console_write("\n");

    start_task(current->regs.esp, current->regs.eip);

    console_write("scheduler_start: returned unexpectedly!\n");
    for (;;) __asm__ volatile("hlt");
}
