#pragma once
#include <stdint.h>

void kmalloc_init(void);
void* kmalloc(uint32_t size);
