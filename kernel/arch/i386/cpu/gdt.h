#pragma once
#include <stdint.h>

/* -------------------- GDT structures -------------------- */

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* -------------------- TSS structure --------------------- */
/* Hardware Task State Segment (32-bit) */

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

/* -------------------- API ------------------------------- */

void gdt_init(void);

/* Allow kernel to update esp0 if we later have per-task stacks */
void tss_set_kernel_stack(uint32_t stack_top);

/* -------------------- Segment selectors ----------------- */

#define GDT_ENTRY_NULL      0
#define GDT_ENTRY_KCODE     1
#define GDT_ENTRY_KDATA     2
#define GDT_ENTRY_UCODE     3
#define GDT_ENTRY_UDATA     4
#define GDT_ENTRY_TSS       5

/* Ring 0 selectors */
#define KERNEL_CODE_SEG  (GDT_ENTRY_KCODE << 3)
#define KERNEL_DATA_SEG  (GDT_ENTRY_KDATA << 3)

/* Ring 3 selectors (RPL = 3) */
#define USER_CODE_SEG    ((GDT_ENTRY_UCODE << 3) | 0x3)
#define USER_DATA_SEG    ((GDT_ENTRY_UDATA << 3) | 0x3)

/* TSS selector (ring 0) */
#define TSS_SEG          (GDT_ENTRY_TSS << 3)
