#pragma once
#include <stdint.h>

#define PAGE_PRESENT  0x001
#define PAGE_RW       0x002
#define PAGE_USER     0x004

void paging_init(void);
void paging_enable(void);

/* Map/unmap a single 4KB page (within the initially mapped 0–64MB range) */
void paging_map(uint32_t vaddr, uint32_t paddr, uint32_t flags);
void paging_unmap(uint32_t vaddr);
