#ifndef _LIBRARY_H
#define _LIBRARY_H
#include "stm32f103xb.h"
#include <stdbool.h>
#include <stdint.h>


void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);
void mini_printf(const char *fmt, ...);
#endif // _LIBRARY_H