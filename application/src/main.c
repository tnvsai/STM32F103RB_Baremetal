#include "stm32f103xb.h"
#include "utility.h"
#include"uart.h"
#include "timer.h"
#include "gpio.h"
#include "rcc.h"

void LED1_Callback(void) { 
     LED_Toggle();
    UART_WriteString(USART2, "interupt hit \r\n");
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
    // Enable clocks for GPIOC, GPIOA, and AFIO peripherals
    RCC_EnableClock(RCC_APB2, RCC_IOPCEN | RCC_IOPAEN | RCC_AFIOEN);
    
    TIMER_InitMs(TIMER2, 1000, LED1_Callback);
    TIMER_EnableInterrupt(TIMER2);

    // Initialize UART2 (GPIO pins automatically configured)
    UART_Init(USART2, &uart2_cfg);

    // Send welcome message
    UART_WriteString(USART2, "we are in application \r\n");
    TIMER_Start(TIMER2);

    while (1) 
    {
    }
}
