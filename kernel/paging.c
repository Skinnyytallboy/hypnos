#include "paging.h"
#include "console.h"

#define PAGE_SIZE 4096

/* Page directory and first page table must be page-aligned */
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void paging_init(void)
{
    console_write("Setting up page tables...\n");

    // Identity map 0–4MB (1024 * 4KB = 4MB)
    for (uint32_t i = 0; i < 1024; i++) {
        // Present | RW | identity-mapped frame
        first_page_table[i] = (i * 0x1000) | 3;
    }

    // Point page directory entry 0 → first page table
    page_directory[0] = ((uint32_t)first_page_table) | 3;

    // Mark the rest not present
    for (uint32_t i = 1; i < 1024; i++) {
        page_directory[i] = 0 | 2; // RW but not present
    }

    console_write("Page tables setup complete.\n");
}

void paging_enable(void)
{
    uint32_t pd_addr = (uint32_t)page_directory;

    // Load page directory address into CR3
    __asm__ volatile("mov %0, %%cr3" :: "r"(pd_addr));

    // Enable paging (bit31 of CR0)
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;       // set PG bit
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

    console_write("Paging enabled.\n");
}
