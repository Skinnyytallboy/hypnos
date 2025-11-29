#pragma once
#include <stdint.h>

extern volatile uint32_t timer_ticks;

void timer_install(void);
uint32_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);