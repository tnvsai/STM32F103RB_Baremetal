#include "stm32f103xb.h"
#include "uart.h"
#include "rcc.h"
#include "gpio.h"

// Crude delay (~1 second at 8 MHz)
static void delay(volatile uint32_t count) {
    while (count--) __asm__("nop");
}

int main(void) {
        // UART2 configuration
    UART_Config_t uart2_cfg = {
        .baudRate   = 115200,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits   = UART_STOPBITS_1,
        .parity     = UART_PARITY_NONE,
        .enableTx   = 1,
        .enableRx   = 1
    };
    

    // Initialize UART2 (GPIO pins automatically configured)
    UART_Init(USART2, &uart2_cfg);

    // Enable GPIOC + GPIOA clocks
    RCC_EnableClock(RCC_APB2, RCC_IOPCEN | RCC_IOPAEN | RCC_AFIOEN);

    // Configure PC13 as push-pull output
    GPIO_ConfigPin(GPIOA, 5, GPIO_OUTPUT_2M, GPIO_GP_PUSH_PULL);

    // Configure PA0 as input with pull-up
    GPIO_ConfigInput(GPIOC, 13, GPIO_PULLUP);

    while (1) {
        if (GPIO_ReadPin(GPIOC, 13) == 0) { // Button pressed
\
            UART_WriteString(USART2, "Button pressed\r\n");
            GPIO_WritePin(GPIOA, 5, 0);   // LED off
            delay(500000);
            UART_WriteString(USART2, "LED OFF\r\n");
        } else {
            UART_WriteString(USART2, "Button released \r\n");
            GPIO_WritePin(GPIOA, 5, 1);   // LED on
             delay(500000);
            UART_WriteString(USART2, "LED ON\r\n");
        }
    }
}


