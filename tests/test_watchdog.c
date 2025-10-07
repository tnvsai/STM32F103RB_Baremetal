#include "stm32f103xb.h"
#include "watchdog.h"
#include "uart.h"
#include "gpio.h"
#include "rcc.h"

#define LED_PIN 5
#define LED_PORT GPIOA

void delay_ms(uint32_t ms) {
    for(uint32_t i=0;i<ms*4000;i++) __NOP();
}

void uart_print(const char *msg) {
    UART_WriteString(USART2,msg);
    UART_WriteString(USART2,"\r\n");
    delay_ms(2);
}

int main(void) {
    // ---------------- RCC & GPIO & UART ----------------
    RCC_EnableClock(RCC_APB2, RCC_IOPAEN | RCC_AFIOEN);
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    GPIO_ConfigPin(LED_PORT, LED_PIN, GPIO_OUTPUT_2M, GPIO_GP_PUSH_PULL);

    UART_Config_t uart2_cfg = {115200, UART_WORDLENGTH_8B, UART_STOPBITS_1,
                               UART_PARITY_NONE, 1, 1};
    UART_Init(USART2, &uart2_cfg);

    // ---------------- Check reset cause ----------------
    if (RCC->CSR & RCC_CSR_IWDGRSTF) {
        RCC->CSR |= RCC_CSR_RMVF;       // Clear reset flags
        uart_print("MCU reset by IWDG!");
        GPIO_WritePin(LED_PORT, LED_PIN, 1); // LED ON to indicate reset
        delay_ms(500);
        GPIO_WritePin(LED_PORT, LED_PIN, 0); // Turn OFF
    }

    if (RCC->CSR & RCC_CSR_WWDGRSTF) {
        RCC->CSR |= RCC_CSR_RMVF;       // Clear reset flags
        uart_print("MCU reset by WWDG!");
        GPIO_WritePin(LED_PORT, LED_PIN, 1); // LED ON to indicate reset
        delay_ms(500);
        GPIO_WritePin(LED_PORT, LED_PIN, 0); // Turn OFF
    }

    uart_print("UART ready");

    // ---------------- Start IWDG test ----------------
    uart_print("Starting IWDG test (expect reset in ~5s)");
    IWDG_Init(5000);  // 5s timeout

    while(1) {
        // Do nothing, wait for IWDG to reset MCU
    }
}
