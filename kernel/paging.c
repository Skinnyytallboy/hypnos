#include <stdint.h>
#include "paging.h"
#include "console.h"

#define PAGE_SIZE        4096
#define MAX_IDENTITY_MB  64
#define NUM_PAGE_TABLES  (MAX_IDENTITY_MB / 4)   // 4MB per page table

static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_tables[NUM_PAGE_TABLES][1024] __attribute__((aligned(4096)));

static inline uint32_t pde_index(uint32_t addr) { return addr >> 22; }
static inline uint32_t pte_index(uint32_t addr) { return (addr >> 12) & 0x3FF; }

static inline uint32_t* get_pte(uint32_t vaddr)
{
    uint32_t pdi = pde_index(vaddr);
    uint32_t pti = pte_index(vaddr);

    if (pdi >= NUM_PAGE_TABLES)
        return 0;  // out of our identity-mapped region

    return &page_tables[pdi][pti];
}

void paging_init(void)
{
    console_write("Setting up identity-mapped paging (0–64MB)...\n");

    // Clear page directory
    for (uint32_t i = 0; i < 1024; i++) {
        page_directory[i] = 0;
    }

    // Build 16 page tables: 16 * 4MB = 64MB
    uint32_t addr = 0;
    for (uint32_t t = 0; t < NUM_PAGE_TABLES; t++) {
        for (uint32_t i = 0; i < 1024; i++) {
            // identity map: vaddr == paddr
            page_tables[t][i] = (addr & ~0xFFF) | (PAGE_PRESENT | PAGE_RW);
            addr += PAGE_SIZE;
        }

        // point PDE[t] to page_tables[t]
        page_directory[t] =
            ((uint32_t)&page_tables[t]) | (PAGE_PRESENT | PAGE_RW);
    }

    // rest of PDE entries: not present
    for (uint32_t t = NUM_PAGE_TABLES; t < 1024; t++) {
        page_directory[t] = PAGE_RW;  // RW but not present
    }

    console_write("Identity map complete.\n");
}

void paging_enable(void)
{
    uint32_t pd_addr = (uint32_t)page_directory;

    __asm__ volatile("mov %0, %%cr3" :: "r"(pd_addr));

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;   // set PG bit
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

    console_write("Paging enabled.\n");
}

/* Map a single page (vaddr -> paddr) inside 0–64MB region */
void paging_map(uint32_t vaddr, uint32_t paddr, uint32_t flags)
{
    uint32_t* pte = get_pte(vaddr);
    if (!pte) {
        console_write("paging_map: vaddr out of initial range.\n");
        return;
    }

    *pte = (paddr & ~0xFFF) | (flags & 0xFFF);

    // flush TLB for that page
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

/* Unmap a single page in 0–64MB region */
void paging_unmap(uint32_t vaddr)
{
    uint32_t* pte = get_pte(vaddr);
    if (!pte) return;

    *pte = 0;
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}
