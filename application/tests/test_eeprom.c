#include "stm32f103xb.h"
#include "uart.h"
#include "i2c.h"
#include "eeprom.h"
#include <stdio.h>

#define TEST_MEM_ADDR 0x0000
#define TEST_STR_LEN  7

uint8_t write_data[TEST_STR_LEN] = {'S','r','i','R','a','m','a'};
uint8_t read_data[TEST_STR_LEN];

int main(void) {
    char buf[50];

    // -----------------------------
    // Initialize UART2
    // -----------------------------
    UART_Config_t uart2_cfg = {
        .baudRate   = 115200,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits   = UART_STOPBITS_1,
        .parity     = UART_PARITY_NONE,
        .enableTx   = 1,
        .enableRx   = 0
    };
    UART_Init(USART2, &uart2_cfg);
    UART_WriteString(USART2, "USART2 ready!\r\n");

    // -----------------------------
    // Initialize I2C
    // -----------------------------
    UART_WriteString(USART2, "Initializing I2C1...\r\n");
    if(I2C_Init(I2C1, 100000) != I2C_OK) {
        UART_WriteString(USART2, "I2C Init failed!\r\n");
        while(1);
    }
    UART_WriteString(USART2, "I2C Init success.\r\n");

    // -----------------------------
    // Single-byte write/read test
    // -----------------------------
    UART_WriteString(USART2, "Testing single-byte write/read...\r\n");

    EEPROM_WriteByte(I2C1, TEST_MEM_ADDR, write_data[0]);

    uint8_t byte = 0;
    EEPROM_ReadByte(I2C1, TEST_MEM_ADDR, &byte);
    sprintf(buf, "Read byte: %c (0x%02X)\r\n", byte, byte);
    UART_WriteString(USART2, buf);

    // -----------------------------
    // Multi-byte write
    // -----------------------------
    UART_WriteString(USART2, "Testing multi-byte write...\r\n");
    for(int i=0;i<TEST_STR_LEN;i++){
        sprintf(buf, "Writing byte '%c'...\r\n", write_data[i]);
        UART_WriteString(USART2, buf);
    }

    EEPROM_WriteBytes(I2C1, TEST_MEM_ADDR, write_data, TEST_STR_LEN);
    UART_WriteString(USART2, "EEPROM multi-byte write completed.\r\n");

    // -----------------------------
    // Multi-byte read
    // -----------------------------
    UART_WriteString(USART2, "Testing multi-byte read...\r\n");
    EEPROM_ReadBytes(I2C1, TEST_MEM_ADDR, read_data, TEST_STR_LEN);

    UART_WriteString(USART2, "Read data: ");
    for(int i=0;i<TEST_STR_LEN;i++){
        sprintf(buf, "%c", read_data[i]);
        UART_WriteString(USART2, buf);
    }
    UART_WriteString(USART2, "\r\n");

    // -----------------------------
    // Data verification
    // -----------------------------
    int match = 1;
    for(int i=0;i<TEST_STR_LEN;i++) if(read_data[i] != write_data[i]) match = 0;

    if(match) UART_WriteString(USART2, "Data verification PASSED!\r\n");
    else      UART_WriteString(USART2, "Data verification FAILED!\r\n");

    UART_WriteString(USART2, "All EEPROM API tests completed.\r\n");

    while(1);
}
