#include "display_st7789.h"
#include "stm32f103xb.h"
#include "spi.h"
#include "utility.h"   // mini_printf for logging
#include <stdint.h>

// ===============================
// --- SPI Communication Helpers ---
// ===============================

// Send a command to the ST7789
// DC pin LOW indicates this is a command
static void ST7789_WriteCommand(uint8_t cmd) {
    ST7789_DC_LOW();     // Set DC pin low → next byte is command
    SPI2_Transmit(cmd);  // Send the command via SPI2
}

// Send a single data byte to ST7789
// DC pin HIGH indicates this is data
static void ST7789_WriteData(uint8_t data) {
    ST7789_DC_HIGH();    // Set DC pin high → next byte is data
    SPI2_Transmit(data); // Send the data via SPI2
}

// Send multiple data bytes from a buffer
static void ST7789_WriteDataBuffer(uint8_t *buff, uint16_t len) {
    ST7789_DC_HIGH();    // Data mode
    for(uint16_t i=0;i<len;i++)
        SPI2_Transmit(buff[i]);
}

// ===============================
// --- GPIO Initialization ---
// ===============================

// Initialize GPIO pins connected to ST7789 control signals
// PB11 → DC, PB12 → RESET
void ST7789_GPIO_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // Enable clock for GPIOB

    // Configure PB11 and PB12 as output push-pull, max speed 50MHz
    GPIOB->CRH &= ~((0xF << ((11-8)*4)) | (0xF << ((12-8)*4))); // clear
    GPIOB->CRH |= ((0x3 << ((11-8)*4)) | (0x3 << ((12-8)*4)));  // set as output
}

// ===============================
// --- Reset the Display ---
// ===============================

// Hardware reset sequence for ST7789
void ST7789_Reset(void) {
    ST7789_RST_LOW(); // Pull RESET low
    for(volatile int i=0;i<50000;i++); // Short delay
    ST7789_RST_HIGH(); // Pull RESET high
    for(volatile int i=0;i<50000;i++); // Short delay
    mini_printf("ST7789 Reset Done\r\n");
}

// ===============================
// --- Address Window ---
// ===============================

// Set the rectangular area (x0,y0,x1,y1) where pixel data will be written
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4];

    // Column Address Set
    ST7789_WriteCommand(0x2A); // Column command
    data[0] = x0 >> 8; data[1] = x0 & 0xFF; // High byte, low byte of start
    data[2] = x1 >> 8; data[3] = x1 & 0xFF; // High byte, low byte of end
    ST7789_WriteDataBuffer(data, 4);

    // Row Address Set
    ST7789_WriteCommand(0x2B); // Row command
    data[0] = y0 >> 8; data[1] = y0 & 0xFF;
    data[2] = y1 >> 8; data[3] = y1 & 0xFF;
    ST7789_WriteDataBuffer(data, 4);

    // Memory Write command to start writing pixel data
    ST7789_WriteCommand(0x2C);
}

// ===============================
// --- Initialization ---
// ===============================

void ST7789_Init(void) {
    ST7789_GPIO_Init(); // Init DC/RESET pins
    SPI2_Init();        // Init SPI2 peripheral
    ST7789_Reset();     // Reset display

    // Memory Data Access Control
    ST7789_WriteCommand(0x36); // MADCTL
    ST7789_WriteData(0x00);    // RGB order, no rotation

    // Color mode setup
    ST7789_WriteCommand(0x3A); // COLMOD
    ST7789_WriteData(0x05);    // 16-bit/pixel (RGB565)

    // Exit sleep mode
    ST7789_WriteCommand(0x11); // Sleep Out
    for(volatile int i=0;i<50000;i++); // small delay

    // Turn display ON
    ST7789_WriteCommand(0x29); // Display ON
    mini_printf("ST7789 Initialized\r\n");
}

// ===============================
// --- Draw Pixel ---
// ===============================

// Draw a single pixel at (x,y) with 16-bit color
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if(x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return; // bounds check

    ST7789_SetAddressWindow(x, y, x, y); // set window to a single pixel
    uint8_t data[2] = {color >> 8, color & 0xFF}; // convert 16-bit color to 2 bytes
    ST7789_WriteDataBuffer(data, 2); // write pixel
}

// ===============================
// --- Fill Screen ---
// ===============================

// Fill entire screen with a single color
void ST7789_FillScreen(uint16_t color) {
    ST7789_FillRect(0, 0, ST7789_WIDTH, ST7789_HEIGHT, color);
}

// ===============================
// --- Fill Rectangle ---
// ===============================

// Fill a rectangle area with a specific color
void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if(x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;
    if(x + w > ST7789_WIDTH) w = ST7789_WIDTH - x;   // clamp width
    if(y + h > ST7789_HEIGHT) h = ST7789_HEIGHT - y; // clamp height

    ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1); // set pixel window

    uint8_t data[2] = {color >> 8, color & 0xFF};
    for(uint32_t i=0;i<w*h;i++)
        ST7789_WriteDataBuffer(data, 2); // fill each pixel
}

// ===============================
// --- 5x7 Font ---
// ===============================

// ASCII 5x7 font table, each char is 5 bytes (5 columns, 7 rows)
static const uint8_t Font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' ' 32
    {0x00,0x00,0x5F,0x00,0x00}, // '!' 33
    {0x00,0x07,0x00,0x07,0x00}, // '"' 34
    {0x14,0x7F,0x14,0x7F,0x14}, // '#' 35
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$' 36
    {0x23,0x13,0x08,0x64,0x62}, // '%' 37
    {0x36,0x49,0x55,0x22,0x50}, // '&' 38
    {0x00,0x05,0x03,0x00,0x00}, // ''' 39
    {0x00,0x1C,0x22,0x41,0x00}, // '(' 40
    {0x00,0x41,0x22,0x1C,0x00}, // ')' 41
    {0x14,0x08,0x3E,0x08,0x14}, // '*' 42
    {0x08,0x08,0x3E,0x08,0x08}, // '+' 43
    {0x00,0x50,0x30,0x00,0x00}, // ',' 44
    {0x08,0x08,0x08,0x08,0x08}, // '-' 45
    {0x00,0x60,0x60,0x00,0x00}, // '.' 46
    {0x20,0x10,0x08,0x04,0x02}, // '/' 47
    {0x3E,0x51,0x49,0x45,0x3E}, // '0' 48
    {0x00,0x42,0x7F,0x40,0x00}, // '1' 49
    {0x42,0x61,0x51,0x49,0x46}, // '2' 50
    {0x21,0x41,0x45,0x4B,0x31}, // '3' 51
    {0x18,0x14,0x12,0x7F,0x10}, // '4' 52
    {0x27,0x45,0x45,0x45,0x39}, // '5' 53
    {0x3C,0x4A,0x49,0x49,0x30}, // '6' 54
    {0x01,0x71,0x09,0x05,0x03}, // '7' 55
    {0x36,0x49,0x49,0x49,0x36}, // '8' 56
    {0x06,0x49,0x49,0x29,0x1E}, // '9' 57
    {0x00,0x36,0x36,0x00,0x00}, // ':' 58
    {0x00,0x56,0x36,0x00,0x00}, // ';' 59
    {0x08,0x14,0x22,0x41,0x00}, // '<' 60
    {0x14,0x14,0x14,0x14,0x14}, // '=' 61
    {0x00,0x41,0x22,0x14,0x08}, // '>' 62
    {0x02,0x01,0x51,0x09,0x06}, // '?' 63
    {0x32,0x49,0x79,0x41,0x3E}, // '@' 64
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A' 65
    {0x7F,0x49,0x49,0x49,0x36}, // 'B' 66
    {0x3E,0x41,0x41,0x41,0x22}, // 'C' 67
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D' 68
    {0x7F,0x49,0x49,0x49,0x41}, // 'E' 69
    {0x7F,0x09,0x09,0x09,0x01}, // 'F' 70
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G' 71
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H' 72
    {0x00,0x41,0x7F,0x41,0x00}, // 'I' 73
    {0x20,0x40,0x41,0x3F,0x01}, // 'J' 74
    {0x7F,0x08,0x14,0x22,0x41}, // 'K' 75
    {0x7F,0x40,0x40,0x40,0x40}, // 'L' 76
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M' 77
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N' 78
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O' 79
    {0x7F,0x09,0x09,0x09,0x06}, // 'P' 80
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q' 81
    {0x7F,0x09,0x19,0x29,0x46}, // 'R' 82
    {0x46,0x49,0x49,0x49,0x31}, // 'S' 83
    {0x01,0x01,0x7F,0x01,0x01}, // 'T' 84
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U' 85
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V' 86
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W' 87
    {0x63,0x14,0x08,0x14,0x63}, // 'X' 88
    {0x07,0x08,0x70,0x08,0x07}, // 'Y' 89
    {0x61,0x51,0x49,0x45,0x43}, // 'Z' 90
    {0x00,0x7F,0x41,0x41,0x00}, // '[' 91
    {0x02,0x04,0x08,0x10,0x20}, // '\' 92
    {0x00,0x41,0x41,0x7F,0x00}, // ']' 93
    {0x04,0x02,0x01,0x02,0x04}, // '^' 94
    {0x40,0x40,0x40,0x40,0x40}, // '_' 95
    {0x00,0x01,0x02,0x04,0x00} // '`' 96
};

// ===============================
// --- Draw Character (5x7) ---
// ===============================

void ST7789_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg) {
    if(c < 32 || c > 127) return; // ignore non-printable characters
    const uint8_t *bitmap = Font5x7[c - 32]; // get char bitmap

    // Draw 5x7 pixels for character
    for(uint8_t i=0;i<5;i++) {
        uint8_t line = bitmap[i];
        for(uint8_t j=0;j<8;j++) { // 7 rows + 1 extra bit
            ST7789_DrawPixel(x+i, y+j, (line & 0x1) ? color : bg);
            line >>= 1; // next row
        }
    }
}

// ===============================
// --- Draw String ---
// ===============================

void ST7789_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg) {
    while(*str) {
        ST7789_DrawChar(x, y, *str, color, bg);
        x += 6; // 5 pixels + 1 spacing
        str++;
        if(x + 5 >= ST7789_WIDTH) { x = 0; y += 8; } // line wrap
        if(y + 7 >= ST7789_HEIGHT) break;           // bottom of screen
    }
}

// ===============================
// --- Draw Character Scaled ---
// ===============================

void ST7789_DrawCharScaled(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t scale) {
    if(c < 32 || c > 127) return;
    const uint8_t *bitmap = Font5x7[c - 32];

    for(uint8_t col = 0; col < 5; col++) {
        uint8_t bits = bitmap[col];
        for(uint8_t row = 0; row < 7; row++) {
            uint16_t fill = (bits & (1 << row)) ? color : bg;
            // Fill a block of 'scale x scale' pixels
            ST7789_FillRect(x + col*scale, y + row*scale, scale, scale, fill);
        }
    }
}

// ===============================
// --- Draw String Scaled ---
// ===============================

void ST7789_DrawStringScaled(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale) {
    while(*str) {
        ST7789_DrawCharScaled(x, y, *str, color, bg, scale);
        x += 6 * scale; // move to next char
        str++;
        if(x + 5*scale >= ST7789_WIDTH) { x = 0; y += 8*scale; } // line wrap
        if(y + 7*scale >= ST7789_HEIGHT) break;                  // bottom of screen
    }
}
