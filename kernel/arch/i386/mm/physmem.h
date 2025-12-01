#pragma once
#include <stdint.h>

void phys_init(void);
uint32_t phys_alloc_frame(void);
void phys_free_frame(uint32_t addr);
