#ifndef _UTILITY_H
#define _UTILITY_H
#include "stm32f103xb.h"
#include <stdbool.h>
#include <stdint.h>


void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);
void mini_printf(const char *fmt, ...);
#endif // _UTILITY_H