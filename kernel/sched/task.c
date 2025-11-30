#include "task.h"
#include "arch/i386/mm/kmalloc.h"
#include "console.h"

extern void start_task(uint32_t new_esp, uint32_t new_ebp, uint32_t new_eip);
extern void switch_task(uint32_t*, uint32_t*, uint32_t*,
                        uint32_t,  uint32_t,  uint32_t);

static task_t* current_task = 0;
static task_t* task_list = 0;
static uint32_t next_pid = 1;

/* All threads start here */
static void task_wrapper(void (*func)(void))
{
    func();  // run the actual thread body

    console_write("\n[thread finished]\n");
    while (1) {
        task_yield();  // never return into invalid EIP
    }
}

void tasking_init(void)
{
    console_write("Initializing tasking...\n");
    current_task = 0;
    task_list = 0;
    next_pid = 1;
    console_write("Tasking initialized.\n");
}

void task_create(void (*func)(void))
{
    console_write("Creating new kernel thread...\n");

    task_t* new_task = (task_t*) kmalloc(sizeof(task_t));
    new_task->id = next_pid++;

    uint32_t* stack = kmalloc(4096);   // 4KB stack
    new_task->stack = stack;

    uint32_t esp = (uint32_t)stack + 4096;

    /*
     * Build a fake stack frame for:
     *   task_wrapper(func);
     *
     * Layout we want at entry to task_wrapper:
     *   [esp+0] = fake return address (0)
     *   [esp+4] = func (argument)
     */

    esp -= 4;
    *(uint32_t*)esp = (uint32_t)func;   // argument

    esp -= 4;
    *(uint32_t*)esp = 0;                // fake return address

    new_task->eip = (uint32_t)task_wrapper;
    new_task->esp = esp;
    new_task->ebp = esp;

    if (!task_list) {
        task_list = new_task;
        new_task->next = new_task;      // circular list
    } else {
        new_task->next = task_list->next;
        task_list->next = new_task;
    }
}

void task_start(void)
{
    if (!task_list) {
        console_write("task_start: no tasks to run.\n");
        return;
    }

    current_task = task_list;
    console_write("Starting first task...\n");
    start_task(current_task->esp, current_task->ebp, current_task->eip);

    // never returns
}

void task_yield(void)
{
    if (!current_task) return;

    task_t* prev = current_task;
    task_t* next = current_task->next;
    current_task = next;

    switch_task(&prev->esp, &prev->ebp, &prev->eip,
                next->esp,  next->ebp,  next->eip);
}
