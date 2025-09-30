#ifndef UART_H
#define UART_H

#include "stm32f103xb.h"

typedef enum {
    UART_WORDLENGTH_8B = 0,
    UART_WORDLENGTH_9B = 1
} UART_WordLength_t;

typedef enum {
    UART_STOPBITS_1 = 0,
    UART_STOPBITS_2 = 2
} UART_StopBits_t;

typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
} UART_Parity_t;

typedef struct {
    uint32_t baudRate;
    UART_WordLength_t wordLength;
    UART_StopBits_t stopBits;
    UART_Parity_t parity;
    uint8_t enableTx;
    uint8_t enableRx;
} UART_Config_t;

void UART_Init(USART_TypeDef *USARTx, UART_Config_t *config);
void UART_WriteChar(USART_TypeDef *USARTx, char c);
void UART_WriteString(USART_TypeDef *USARTx, const char *str);
char UART_ReadChar(USART_TypeDef *USARTx);

#endif
