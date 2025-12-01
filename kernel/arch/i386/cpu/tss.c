#include <stdint.h>
#include "tss.h"
#include "gdt.h"

static struct tss_entry tss;

void tss_set_kernel_stack(uint32_t stack)
{
    tss.esp0 = stack;
}

void tss_init(void)
{
    // Zero TSS
    uint8_t* p = (uint8_t*)&tss;
    for (int i = 0; i < sizeof(tss); i++) p[i] = 0;

    tss.ss0  = 0x10;   // kernel data selector
    tss.esp0 = 0;      // set later

    gdt_set_tss(5, (uint32_t)&tss, sizeof(tss));  // GDT entry #5 dedicated for TSS

    load_tss();  // executes "ltr"
}
