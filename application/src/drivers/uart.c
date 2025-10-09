// uart.c
#include "uart.h"

// Helper: get APB clock for USARTx
static uint32_t UART_GetClock(USART_TypeDef *USARTx) {
    uint32_t pclk1 = 8000000UL; // default HSI
    uint32_t pclk2 = 8000000UL;

    // Determine HCLK from RCC_CFGR
    uint32_t sws = (RCC->CFGR >> 2) & 0x3;
    if (sws == 0) pclk2 = pclk1 = 8000000UL;      // HSI
    else if (sws == 1) pclk2 = pclk1 = 8000000UL; // HSE 8MHz
    else if (sws == 2) pclk2 = 72000000UL;        // PLL 72MHz
    // APB1 prescaler
    uint32_t ppre1 = (RCC->CFGR >> 8) & 0x7;
    uint32_t ppre2 = (RCC->CFGR >> 11) & 0x7;
    if (ppre1 >= 4) pclk1 >>= (ppre1 - 3);
    if (ppre2 >= 4) pclk2 >>= (ppre2 - 3);

    return (USARTx == USART1) ? pclk2 : pclk1;
}

// Helper: set baud rate
static void UART_SetBaudRate(USART_TypeDef *USARTx, uint32_t baudRate) {
    uint32_t pclk = UART_GetClock(USARTx);
    USARTx->BRR = (pclk + (baudRate/2U)) / baudRate;
}

// Helper: configure GPIO for given USART
static void UART_ConfigGPIO(USART_TypeDef *USARTx) {
    if (USARTx == USART1) {
        // Enable GPIOA clock
        RCC->APB2ENR |= (1 << 2);
        // PA9 = TX, AF push-pull
        GPIOA->CRH &= ~(0xF << ((9-8)*4));
        GPIOA->CRH |=  (0xB << ((9-8)*4));
        // PA10 = RX, floating input
        GPIOA->CRH &= ~(0xF << ((10-8)*4));
        GPIOA->CRH |=  (0x4 << ((10-8)*4));
    } else if (USARTx == USART2) {
        // Enable GPIOA clock
        RCC->APB2ENR |= (1 << 2);
        // PA2 = TX
        GPIOA->CRL &= ~(0xF << (2*4));
        GPIOA->CRL |=  (0xB << (2*4));
        // PA3 = RX
        GPIOA->CRL &= ~(0xF << (3*4));
        GPIOA->CRL |=  (0x4 << (3*4));
    } else if (USARTx == USART3) {
        // Enable GPIOB clock
        RCC->APB2ENR |= (1 << 3);
        // PB10 = TX
        GPIOB->CRH &= ~(0xF << ((10-8)*4));
        GPIOB->CRH |=  (0xB << ((10-8)*4));
        // PB11 = RX
        GPIOB->CRH &= ~(0xF << ((11-8)*4));
        GPIOB->CRH |=  (0x4 << ((11-8)*4));
    }
}

void UART_Init(USART_TypeDef *USARTx, UART_Config_t *config) {
    // 1. Enable USART clock
    if (USARTx == USART1) RCC->APB2ENR |= (1 << 14);
    if (USARTx == USART2) RCC->APB1ENR |= (1 << 17);
    if (USARTx == USART3) RCC->APB1ENR |= (1 << 18);

    // 2. Configure GPIO
    UART_ConfigGPIO(USARTx);

    // 3. Configure baud rate
    UART_SetBaudRate(USARTx, config->baudRate);

    // 4. Configure word length
    if (config->wordLength == UART_WORDLENGTH_9B)
        USARTx->CR1 |= (1 << 12);
    else
        USARTx->CR1 &= ~(1 << 12);

    // 5. Configure stop bits
    USARTx->CR2 &= ~(3 << 12);
    USARTx->CR2 |= (config->stopBits << 12);

    // 6. Configure parity
    USARTx->CR1 &= ~((1 << 10) | (1 << 9));
    if (config->parity == UART_PARITY_EVEN)
        USARTx->CR1 |= (1 << 10);
    else if (config->parity == UART_PARITY_ODD)
        USARTx->CR1 |= (1 << 10) | (1 << 9);

    // 7. Enable TX/RX
    if (config->enableTx) USARTx->CR1 |= (1 << 3);
    if (config->enableRx) USARTx->CR1 |= (1 << 2);

    // 8. Enable USART
    USARTx->CR1 |= (1 << 13);
}

void UART_WriteChar(USART_TypeDef *USARTx, char c) {
    while (!(USARTx->SR & (1 << 7)));
    USARTx->DR = (uint16_t)c;
}

void UART_WriteString(USART_TypeDef *USARTx, const char *str) {
    while (*str) UART_WriteChar(USARTx, *str++);
}

char UART_ReadChar(USART_TypeDef *USARTx) {
    while (!(USARTx->SR & (1 << 5)));
    return (char)(USARTx->DR & 0xFF);
}
