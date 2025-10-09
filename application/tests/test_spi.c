#include "stm32f103xb.h"
#include "spi.h"
#include "uart.h"
#include <stdio.h>

// ----------------- CS Pin -----------------
#define CS_PORT GPIOA
#define CS_PIN  4

void CS_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // Enable GPIOA clock
    GPIOA->CRL &= ~(0xF << (CS_PIN*4)); // Clear config
    GPIOA->CRL |= 0x3 << (CS_PIN*4);    // Output push-pull 10MHz
    GPIOA->BSRR = (1 << CS_PIN);        // idle high
}

void CS_LOW(void)  { GPIOA->BSRR = (1 << (CS_PIN + 16)); }
void CS_HIGH(void) { GPIOA->BSRR = (1 << CS_PIN); }

void delay(volatile uint32_t t) { while(t--); }

int main(void) {
    // ----------------- UART Init -----------------
    UART_Config_t uart2_cfg = {
        .baudRate   = 115200,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits   = UART_STOPBITS_1,
        .parity     = UART_PARITY_NONE,
        .enableTx   = 1,
        .enableRx   = 0
    };
    UART_Init(USART2, &uart2_cfg);
    UART_WriteString(USART2, "USART2 SPI Test Ready!\r\n");

    // ----------------- SPI Init -----------------
    SPI1_GPIO_Init();
    SPI1_Init();

    // ----------------- CS Init -----------------
    CS_Init();

    char buf[50];
    uint8_t txData[] = {0xAA, 0x55, 0xFF, 0x00}; // Test pattern
    uint8_t rx;

    while(1) {
        for(uint8_t i=0; i<sizeof(txData); i++) {
            CS_LOW();
            delay(1000); // tiny delay

            rx = SPI1_Transmit(txData[i]); // send byte

            delay(1000);
            CS_HIGH();

            sprintf(buf, "CS toggle -> Sent: 0x%02X, Received: 0x%02X\r\n", txData[i], rx);
            UART_WriteString(USART2, buf);

            delay(500000); // slow down loop for logic analyzer
        }
    }
}
