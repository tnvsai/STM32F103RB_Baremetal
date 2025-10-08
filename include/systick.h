#ifndef SYSTICK_H
#define SYSTICK_H

#include "stm32f103xb.h"
#include <stdint.h>

void SysTick_Init(uint32_t ticks_per_second);
void SysTick_Handler(void);
void SysTick_DelayMs(uint32_t ms);
uint32_t SysTick_GetTick(void);

#endif
