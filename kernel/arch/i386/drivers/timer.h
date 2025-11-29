#pragma once
#include <stdint.h>

extern volatile uint32_t timer_ticks;

void timer_install(void);
