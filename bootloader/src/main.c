#include "stm32f103xb.h"
#include "uart.h"
#include "flash.h"

#define TEST_WORD1 0x1234
#define TEST_WORD2 0xABCD

int main(void)
{
    // 1. Initialize UART2
    UART_Config_t uart2_cfg = {
        .baudRate   = 115200,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits   = UART_STOPBITS_1,
        .parity     = UART_PARITY_NONE,
        .enableTx   = 1,
        .enableRx   = 1
    };
    UART_Init(USART2, &uart2_cfg);
    UART_WriteString(USART2, "Flash program & verify test start...\r\n");

    // 2. Unlock Flash
    Flash_Unlock();
    UART_WriteString(USART2, "Flash unlocked.\r\n");

    // 3. Erase full app region
    if (Flash_EraseAppRegion() == FLASH_OK) {
        UART_WriteString(USART2, "App region erased successfully.\r\n");
    } else {
        UART_WriteString(USART2, "App region erase FAILED!\r\n");
    }

    // 4. Program TEST_WORD1 and TEST_WORD2
    if (Flash_ProgramHalfWord(FLASH_START_ADDRESS, TEST_WORD1) == FLASH_OK) {
        UART_WriteString(USART2, "TEST_WORD1 programmed.\r\n");
    } else {
        UART_WriteString(USART2, "TEST_WORD1 FAILED!\r\n");
    }

    if (Flash_ProgramHalfWord(FLASH_START_ADDRESS + 2, TEST_WORD2) == FLASH_OK) {
        UART_WriteString(USART2, "TEST_WORD2 programmed.\r\n");
    } else {
        UART_WriteString(USART2, "TEST_WORD2 FAILED!\r\n");
    }

    // 5. Verify flash content
    uint16_t verify_data[2] = {TEST_WORD1, TEST_WORD2};
    UART_WriteString(USART2, "Verifying flash content...\r\n");
    if (Flash_Verify(FLASH_START_ADDRESS, verify_data, 2)) {
        UART_WriteString(USART2, "Flash verification SUCCESS!\r\n");
    } else {
        UART_WriteString(USART2, "Flash verification FAILED!\r\n");
    }

    // 6. Lock Flash
    Flash_Lock();
    UART_WriteString(USART2, "Flash locked.\r\n");

    while (1); // stop here
}
