#ifndef _UTILITY_H
#define _UTILITY_H
#include "stm32f103xb.h"
#include <stdint.h>
#include "uart.h"
#include <stdarg.h>
#include <stdint.h>
#include "display_st7789.h"


// Temporary buffer size for number/string conversion
#define MINI_PRINTF_BUF_SIZE 32
#define TRUE 1
#define FALSE 0

void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);
void mini_printf(const char *fmt, ...);
void ST7789_mini_printf(uint16_t x, uint16_t y, uint16_t color, uint16_t bg, uint8_t scale, const char *fmt, ...);

#endif // _UTILITY_H