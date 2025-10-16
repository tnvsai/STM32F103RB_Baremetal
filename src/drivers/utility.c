#include "utility.h"

#define LED_PIN 5

static uint8_t ledInitilised = FALSE;

// ---------------- GPIO Init ----------------
static void LED_Init(void)
{
    // Enable clock for GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    // Configure PA5 as output push-pull, 2 MHz
    GPIOA->CRL &= ~(0xF << (LED_PIN * 4));
    GPIOA->CRL |= (0x2 << (LED_PIN * 4));
    ledInitilised = TRUE;
}

// ---------------- LED Toggle ----------------
void LED_Toggle(void)
{
    if (!ledInitilised)
        LED_Init();
    GPIOA->ODR ^= (1 << LED_PIN);
}
// ---------------- LED on ----------------
void LED_On(void)
{
    if (!ledInitilised)
        LED_Init();
    GPIOA->ODR |= (1 << LED_PIN);
}
// ---------------- LED on ----------------
void LED_Off(void)
{
    if (!ledInitilised)
        LED_Init();
    GPIOA->ODR &= ~(1 << LED_PIN);
}

// void uart_print(const char *msg) {
//     UART_WriteString(USART2, msg);
//     UART_WriteString(USART2, "\r\n");
//     delay_ms(2); // allow UART flush
// }

#include "uart.h"
#include <stdarg.h>
#include <stdint.h>

static void u32_to_str(uint32_t value, char *buf) {
    // Handle zero explicitly
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    char temp[10];   // max 10 digits for uint32_t
    int i = 0;

    // Extract digits in reverse order
    while (value > 0) {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }

    // Reverse digits directly into output buffer
    for (int j = 0; j < i; j++) {
        buf[j] = temp[i - j - 1];
    }
    buf[i] = '\0';  // Null-terminate
}


static void u32_to_hex(uint32_t value, char *buf) {
    for (int i = 0; i < 8; i++) {
        uint8_t nibble = (value >> (28 - i * 4)) & 0xF;
        buf[i] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    }
    buf[8] = '\0';
}

void mini_printf(const char *fmt, ...) {
    static uint8_t uart_initialized = 0;
    if (!uart_initialized) {
        UART_Config_t uart2_cfg = {
            .baudRate   = 115200,
            .wordLength = UART_WORDLENGTH_8B,
            .stopBits   = UART_STOPBITS_1,
            .parity     = UART_PARITY_NONE,
            .enableTx   = 1,
            .enableRx   = 1
        };
        UART_Init(USART2, &uart2_cfg);
        uart_initialized = 1;
    }

    va_list args;
    va_start(args, fmt);
    char buf[16];

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd': {
                    int val = va_arg(args, int);
                    if (val < 0) {
                        UART_WriteChar(USART2, '-');
                        val = -val;
                    }
                    u32_to_str((uint32_t)val, buf);
                    UART_WriteString(USART2, buf);
                    break;
                }
                case 'u': {
                    uint32_t val = va_arg(args, uint32_t);
                    u32_to_str(val, buf);
                    UART_WriteString(USART2, buf);
                    break;
                }
                case 'x':
                case 'X': {
                    uint32_t val = va_arg(args, uint32_t);
                    u32_to_hex(val, buf);
                    UART_WriteString(USART2, buf);
                    break;
                }
                case 's': {
                    char *str = va_arg(args, char *);
                    UART_WriteString(USART2, str);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    UART_WriteChar(USART2, c);
                    break;
                }
                case '%':
                    UART_WriteChar(USART2, '%');
                    break;
                default:
                    UART_WriteChar(USART2, '?');
                    break;
            }
        } else {
            UART_WriteChar(USART2, *fmt);
        }
        fmt++;
    }

    va_end(args);
}

// Convert signed integer to string
static void i32_to_str(int32_t val, char *buf) {
    if(val < 0) {
        *buf++ = '-';
        u32_to_str(-val, buf);
    } else {
        u32_to_str(val, buf);
    }
}


// Convert float to string with 1 decimal (fixed-point)
static void float_to_str(float val, char *buf) {
    int32_t int_part = (int32_t)val;
    int32_t frac = (int32_t)((val - int_part) * 10); // 1 decimal
    if(frac < 0) frac = -frac;
    i32_to_str(int_part, buf);
    while(*buf) buf++;          // move to end
    *buf++ = '.';
    *buf++ = '0' + frac;
    *buf = 0;
}


void ST7789_mini_printf(uint16_t x, uint16_t y, uint16_t color, uint16_t bg, uint8_t scale, const char *fmt, ...) {
    char buf[16];
    va_list args;
    va_start(args, fmt);

    while(*fmt) {
        if(*fmt == '%') {
            fmt++;
            switch(*fmt) {
                case 'd': {
                    int val = va_arg(args, int);
                    i32_to_str(val, buf);
                    ST7789_DrawStringScaled(x, y, buf, color, bg, scale);
                    x += strlen(buf)*6*scale;
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(args, unsigned int);
                    u32_to_str(val, buf);
                    ST7789_DrawStringScaled(x, y, buf, color, bg, scale);
                    x += strlen(buf)*6*scale;
                    break;
                }
                case 'f': {
                    double val = va_arg(args, double); // float promoted to double
                    float_to_str((float)val, buf);
                    ST7789_DrawStringScaled(x, y, buf, color, bg, scale);
                    x += strlen(buf)*6*scale;
                    break;
                }
                case 's': {
                    char *str = va_arg(args, char*);
                    ST7789_DrawStringScaled(x, y, str, color, bg, scale);
                    x += strlen(str)*6*scale;
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    char ch[2] = {c, 0};
                    ST7789_DrawStringScaled(x, y, ch, color, bg, scale);
                    x += 6*scale;
                    break;
                }
                case '%': {
                    char ch[2] = {'%',0};
                    ST7789_DrawStringScaled(x, y, ch, color, bg, scale);
                    x += 6*scale;
                    break;
                }
                default: break;
            }
        } else {
            char ch[2] = {*fmt,0};
            ST7789_DrawStringScaled(x, y, ch, color, bg, scale);
            x += 6*scale;
        }
        fmt++;
    }

    va_end(args);
}

