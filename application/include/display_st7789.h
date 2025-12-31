#ifndef DISPLAY_ST7789_H
#define DISPLAY_ST7789_H

#include <stdint.h>

// --- GPIO Macros ---
#define ST7789_DC_HIGH()   (GPIOB->BSRR = GPIO_BSRR_BS11)
#define ST7789_DC_LOW()    (GPIOB->BSRR = GPIO_BSRR_BR11)
#define ST7789_RST_HIGH()  (GPIOB->BSRR = GPIO_BSRR_BS12)
#define ST7789_RST_LOW()   (GPIOB->BSRR = GPIO_BSRR_BR12)

// --- Display Dimensions ---
#define ST7789_WIDTH   240
#define ST7789_HEIGHT  240

// --- Color Macros (RGB565) ---
#define ST7789_COLOR_RED     0x07FF
#define ST7789_COLOR_GREEN   0xF81F
#define ST7789_COLOR_BLUE    0xFFE0
#define ST7789_COLOR_BLACK   0xFFFF
#define ST7789_COLOR_WHITE   0x0000
#define ST7789_COLOR_YELLOW  0x001F  // green + blue
#define ST7789_COLOR_CYAN    0xF800  // red + green
#define ST7789_COLOR_MAGENTA 0x07E0  // red + blue

// --- Functions ---
void ST7789_GPIO_Init(void);
void ST7789_Init(void);
void ST7789_Reset(void);
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7789_FillScreen(uint16_t color);
void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);
void ST7789_DrawStringScaled(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale);
void ST7789_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg);
void ST7789_DrawCharScaled(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t scale);
#endif
