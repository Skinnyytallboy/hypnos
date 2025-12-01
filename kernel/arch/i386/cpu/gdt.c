#include <stdint.h>
#include "gdt.h"

extern void gdt_flush(uint32_t);
extern void tss_flush(uint32_t);

/* We now have:
 * 0: null
 * 1: kernel code
 * 2: kernel data
 * 3: user code
 * 4: user data
 * 5: TSS
 */
static struct gdt_entry gdt[6];
static struct gdt_ptr   gp;

static struct tss_entry tss;

static uint8_t kernel_tss_stack[4096];

static void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access      = access;
}

static void write_tss(int num, uint16_t ss0, uint32_t esp0)
{
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = base + sizeof(struct tss_entry);

    gdt_set_entry(num, base, limit,
                  0x89,        /* access */
                  0x40);       /* granularity: 32-bit, byte granularity */

    for (uint32_t *p = (uint32_t*)&tss;
         p < (uint32_t*)(&tss + 1);
         ++p)
    {
        *p = 0;
    }

    tss.ss0  = ss0;   /* Ring 0 stack segment */
    tss.esp0 = esp0;  /* Ring 0 stack pointer */

    /* Set TSS segments – when CPU uses TSS to jump to ring0 it will
       load these segment selectors. Use kernel segments. */
    tss.cs = KERNEL_CODE_SEG | 0;  /* RPL 0 */
    tss.ss = KERNEL_DATA_SEG | 0;
    tss.ds = KERNEL_DATA_SEG | 0;
    tss.es = KERNEL_DATA_SEG | 0;
    tss.fs = KERNEL_DATA_SEG | 0;
    tss.gs = KERNEL_DATA_SEG | 0;

    tss.iomap_base = sizeof(struct tss_entry);
}

void tss_set_kernel_stack(uint32_t stack_top)
{
    tss.esp0 = stack_top;
}

void gdt_init(void)
{
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base  = (uint32_t)&gdt;

    /* 0: null descriptor */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* 1: kernel code segment (ring 0) */
    gdt_set_entry(1,
                  0,
                  0xFFFFFFFF,
                  0x9A,    /* present, ring0, code, executable, readable */
                  0xCF);   /* 4K granularity, 32-bit */

    /* 2: kernel data segment (ring 0) */
    gdt_set_entry(2,
                  0,
                  0xFFFFFFFF,
                  0x92,    /* present, ring0, data, writable */
                  0xCF);

    /* 3: user code segment (ring 3) */
    gdt_set_entry(3,
                  0,
                  0xFFFFFFFF,
                  0xFA,    /* present, ring3, code, executable, readable */
                  0xCF);

    /* 4: user data segment (ring 3) */
    gdt_set_entry(4,
                  0,
                  0xFFFFFFFF,
                  0xF2,    /* present, ring3, data, writable */
                  0xCF);

    /* 5: TSS descriptor (ring 0) */
    uint32_t stack_top = (uint32_t)kernel_tss_stack + sizeof(kernel_tss_stack);
    write_tss(5, KERNEL_DATA_SEG, stack_top);

    /* Load the new GDT */
    gdt_flush((uint32_t)&gp);

    /* Load the TSS (TR register) */
    tss_flush(TSS_SEG);
}
