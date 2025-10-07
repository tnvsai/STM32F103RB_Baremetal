#include "stm32f103xb.h"
#include "uart.h"
#include "bkp.h"
#include <stdio.h>  // <-- for sprintf

UART_Config_t uart2_cfg = {
    .baudRate    = 115200,
    .wordLength  = UART_WORDLENGTH_8B,
    .stopBits    = UART_STOPBITS_1,
    .parity      = UART_PARITY_NONE,
    .enableTx    = 1,
    .enableRx    = 1
};

int main(void)
{
    UART_Init(USART2, &uart2_cfg);
    UART_WriteString(USART2, "\r\nUART ready\r\n");

    BKP_Init();
    UART_WriteString(USART2, "BKP initialized\r\n");

    uint16_t previousValue = BKP_ReadReg(BKP_DR1);

    // ✅ Use sprintf for cleaner formatting
    char msg[64];
    sprintf(msg, "Previous BKP_DR1 value: 0x%04X\r\n", previousValue);
    UART_WriteString(USART2, msg);

    uint16_t newValue = 0x55AA;
    BKP_WriteReg(BKP_DR1, newValue);

    sprintf(msg, "New BKP_DR1 value written: 0x%04X\r\n", newValue);
    UART_WriteString(USART2, msg);

    UART_WriteString(USART2, "Reset the MCU to test persistence\r\n");
    BKP_ResetDomain();

    while (1);
}
