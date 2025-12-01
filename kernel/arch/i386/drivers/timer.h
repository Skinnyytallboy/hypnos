// kernel/arch/i386/drivers/timer.h
#pragma once
#include <stdint.h>

void     timer_install(void);
uint32_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);
