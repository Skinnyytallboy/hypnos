#pragma once
#include <stdint.h>

typedef void (*irq_handler_t)(void);

void irq_install(void);
void irq_register_handler(int irq, irq_handler_t handler);
