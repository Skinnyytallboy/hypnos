#include <stdint.h>
#include "paging.h"
#include "console.h"

#define PAGE_SIZE        4096
#define MAX_IDENTITY_MB  64
#define NUM_PAGE_TABLES  (MAX_IDENTITY_MB / 4)

#define PAGE_PRESENT  0x001
#define PAGE_RW       0x002
#define PAGE_USER     0x004

static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_tables[NUM_PAGE_TABLES][1024] __attribute__((aligned(4096)));

static inline uint32_t pde_index(uint32_t addr) { return addr >> 22; }
static inline uint32_t pte_index(uint32_t addr) { return (addr >> 12) & 0x3FF; }

static inline uint32_t* get_pte(uint32_t vaddr)
{
    uint32_t pdi = pde_index(vaddr);
    uint32_t pti = pte_index(vaddr);

    if (pdi >= NUM_PAGE_TABLES)
        return 0;

    return &page_tables[pdi][pti];
}

void paging_set_user_for_range(uint32_t start, uint32_t end)
{
    if (end < start) return;

    uint32_t s = start & ~0xFFFu;
    uint32_t e = (end + 0xFFFu) & ~0xFFFu;

    for (uint32_t addr = s; addr < e; addr += 0x1000) {
        uint32_t* pte = get_pte(addr);
        if (!pte) continue;

        *pte |= PAGE_USER;
        __asm__ volatile("invlpg (%0)" :: "r"(addr) : "memory");
    }
}

void paging_init(void)
{
    console_write("Setting up identity-mapped paging (0–64MB)...\n");
    for (uint32_t i = 0; i < 1024; i++)
        page_directory[i] = 0;
    /* Build 16 page tables (16 * 4MB = 64MB) */
    uint32_t addr = 0;
    for (uint32_t t = 0; t < NUM_PAGE_TABLES; t++) {
        for (uint32_t i = 0; i < 1024; i++) {
            page_tables[t][i] =
                (addr & ~0xFFFu) |
                (PAGE_PRESENT | PAGE_RW);   // PTE: present, RW (kernel-only for now)
            addr += PAGE_SIZE;
        }
        // IMPORTANT CHANGE: PDE also has PAGE_USER set 
        page_directory[t] =
            ((uint32_t)&page_tables[t]) |
            (PAGE_PRESENT | PAGE_RW | PAGE_USER);
    }
    for (uint32_t t = NUM_PAGE_TABLES; t < 1024; t++)
        page_directory[t] = 0;
    console_write("Identity map complete.\n");
    // Make the first 64MB user-accessible (sets PAGE_USER on PTEs)
    paging_set_user_for_range(0x00000000, 64 * 1024 * 1024);
}

void paging_enable(void)
{
    uint32_t pd_addr = (uint32_t)page_directory;

    __asm__ volatile("mov %0, %%cr3" :: "r"(pd_addr));

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

    console_write("Paging enabled.\n");
}

void paging_map(uint32_t vaddr, uint32_t paddr, uint32_t flags)
{
    uint32_t* pte = get_pte(vaddr);
    if (!pte) {
        console_write("paging_map: vaddr out of initial range.\n");
        return;
    }

    *pte = (paddr & ~0xFFFu) | (flags & 0xFFFu);
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

void paging_unmap(uint32_t vaddr)
{
    uint32_t* pte = get_pte(vaddr);
    if (!pte) return;

    *pte = 0;
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}
