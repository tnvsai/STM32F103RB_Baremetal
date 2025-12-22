#include "stm32f103xb.h"
#include "spi.h"
#include "display_st7789.h"
#include "utility.h"   // for mini_printf
#include <stdint.h>

/*
Display Pin	MCU Pin	Notes
VCC	3.3V	Power supply
GND	GND	Ground
SCL (SCK)	PB13	SPI2 Clock
SDA (MOSI)	PB15	SPI2 MOSI
RESET	PB12	Hardware reset
DC	PB11	Data/Command selection
CS	Not used	Display always selected
LED	3.3V	Backlight

*/

void delay(volatile uint32_t count) {
    while(count--) __NOP();
}

int main(void) {
    // Initialize SPI2 and display
    SPI2_GPIO_Init();
    ST7789_Init();

    mini_printf("=== ST7789 API Test Started ===\r\n");

    // -------------------- 1) Fill Screen Test --------------------
    mini_printf("1) Fill Screen Test: RED\r\n");
    ST7789_FillScreen(ST7789_COLOR_RED);
    delay(500000);

    mini_printf("1) Fill Screen Test: GREEN\r\n");
    ST7789_FillScreen(ST7789_COLOR_GREEN);
    delay(500000);

    mini_printf("1) Fill Screen Test: BLUE\r\n");
    ST7789_FillScreen(ST7789_COLOR_BLUE);
    delay(500000);

    // -------------------- 2) Draw Pixel Test --------------------
    mini_printf("2) Draw Pixel Test\r\n");
    ST7789_FillScreen(ST7789_COLOR_BLACK);
    ST7789_DrawPixel(10, 10, ST7789_COLOR_WHITE);
    ST7789_DrawPixel(50, 50, ST7789_COLOR_RED);
    ST7789_DrawPixel(100, 100, ST7789_COLOR_GREEN);
    ST7789_DrawPixel(150, 150, ST7789_COLOR_BLUE);
    delay(500000);

    // -------------------- 3) Fill Rectangle Test --------------------
    mini_printf("3) Fill Rectangle Test\r\n");
    ST7789_FillRect(20, 20, 50, 30, ST7789_COLOR_YELLOW);
    ST7789_FillRect(80, 60, 100, 50, ST7789_COLOR_CYAN);
    ST7789_FillRect(50, 120, 150, 80, ST7789_COLOR_MAGENTA);
    delay(500000);

    // -------------------- 4) Draw Text Test --------------------
    mini_printf("4) Draw Text Test\r\n");
    ST7789_FillScreen(ST7789_COLOR_BLACK);
    ST7789_DrawStringScaled(10, 10, "JAI SREE RAM", ST7789_COLOR_WHITE, ST7789_COLOR_BLACK, 3);
    ST7789_DrawStringScaled(10, 50, "OM NAMAH SHIVAIAH", ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK, 4);


    delay(500000);

    // -------------------- 4) Dynamic value and text Test --------------------
         mini_printf("5)  Dynamic value and text Testt\r\n");
         for (uint8_t counter=1; counter<6; counter++)
         {
            ST7789_mini_printf(10, 50, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK, 2, "%d: JAI SHREE RAM", counter);
            ST7789_mini_printf(10, 50, ST7789_COLOR_YELLOW, ST7789_COLOR_WHITE, 2, "%d: JAI SHREE RAM", counter);
            for(volatile int i=0; i<10000; i++); // simple delay
            ST7789_mini_printf(10, 50, ST7789_COLOR_CYAN, ST7789_COLOR_BLACK, 2, "%d:OM NAMAH SHIVAIAH", counter);
            ST7789_mini_printf(10, 50, ST7789_COLOR_YELLOW, ST7789_COLOR_GREEN, 2, "%d:OM NAMAH SHIVAIAH", counter);
            for(volatile int i=0; i<100000; i++); // simple delay
         }

    mini_printf("=== ST7789 API Test Finished ===\r\n");

    while(1) {
        // Keep displaying the last screen
        __NOP();
    }
}
