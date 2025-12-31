#include "stm32f103xb.h"
#include "i2c.h"
#include "uart.h"

#define EEPROM_ADDR    0x50       // 24C256 base I2C address
#define TEST_MEM_ADDR  0x0000     // Starting memory address in EEPROM
#define TEST_STR_LEN   7

uint8_t write_data[TEST_STR_LEN] = {'S','r','i','R','a','m','a'};
uint8_t read_data[TEST_STR_LEN];

int main(void) {
    int ret;

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
    ret = I2C_Init(I2C1, 100000);
    if(ret != I2C_OK) {
        UART_WriteString(USART2, "I2C Init failed!\r\n");
        while(1);
    }
    UART_WriteString(USART2, "I2C Init success.\r\n");

    // -----------------------------
    // Single-byte write/read test
    // -----------------------------
    UART_WriteString(USART2, "Testing single-byte write/read...\r\n");

    ret = I2C_Start(I2C1, EEPROM_ADDR, I2C_WRITE);
    if(ret != I2C_OK) UART_WriteString(USART2, "START write failed!\r\n");
    else UART_WriteString(USART2, "START OK.\r\n");

    ret = I2C_Write(I2C1, 0xAA);
    if(ret != I2C_OK) UART_WriteString(USART2, "Single-byte write failed!\r\n");
    else UART_WriteString(USART2, "Single-byte write OK.\r\n");

    I2C_Stop(I2C1);
    UART_WriteString(USART2, "STOP sent after single-byte test.\r\n");

    // Single-byte read (from memory address 0x0000)
    I2C_Start(I2C1, EEPROM_ADDR, I2C_WRITE);
    I2C_Write(I2C1, 0x00); // High byte of memory address
    I2C_Write(I2C1, 0x00); // Low byte
    I2C_Stop(I2C1);

    I2C_Start(I2C1, EEPROM_ADDR, I2C_READ);
    uint8_t byte = I2C_Read(I2C1, 0); // NACK single byte
    I2C_Stop(I2C1);

    char buf[50];
    sprintf(buf, "Read byte: 0x%02X\r\n", byte);
    UART_WriteString(USART2, buf);

    // -----------------------------
    // Multi-byte write
    // -----------------------------
    UART_WriteString(USART2, "Testing multi-byte write...\r\n");

    I2C_Start(I2C1, EEPROM_ADDR, I2C_WRITE);
    I2C_Write(I2C1, (TEST_MEM_ADDR >> 8) & 0xFF); // high byte
    I2C_Write(I2C1, TEST_MEM_ADDR & 0xFF);        // low byte

    for(int i=0;i<TEST_STR_LEN;i++) {
        sprintf(buf, "Writing byte '%c'...\r\n", write_data[i]);
        UART_WriteString(USART2, buf);
        I2C_Write(I2C1, write_data[i]);
    }
    I2C_Stop(I2C1);

    // Wait EEPROM write cycle (~5ms)
    for(volatile int d=0; d<50000; d++);

    UART_WriteString(USART2, "EEPROM multi-byte write completed.\r\n");

    // -----------------------------
    // Multi-byte read
    // -----------------------------
    UART_WriteString(USART2, "Testing multi-byte read...\r\n");

    // Set memory address first
    I2C_Start(I2C1, EEPROM_ADDR, I2C_WRITE);
    I2C_Write(I2C1, (TEST_MEM_ADDR >> 8) & 0xFF);
    I2C_Write(I2C1, TEST_MEM_ADDR & 0xFF);
    I2C_Stop(I2C1);

    // Read bytes
    I2C_Start(I2C1, EEPROM_ADDR, I2C_READ);
    for(int i=0;i<TEST_STR_LEN;i++) {
        read_data[i] = I2C_Read(I2C1, (i<TEST_STR_LEN-1)?1:0); // ACK all but last
    }
    I2C_Stop(I2C1);

    UART_WriteString(USART2, "Read data: ");
    for(int i=0;i<TEST_STR_LEN;i++) {
        sprintf(buf, "%c", read_data[i]);
        UART_WriteString(USART2, buf);
    }
    UART_WriteString(USART2, "\r\n");

    // Verify
    int match = 1;
    for(int i=0;i<TEST_STR_LEN;i++) if(read_data[i] != write_data[i]) match=0;

    if(match) UART_WriteString(USART2, "Data verification PASSED!\r\n");
    else      UART_WriteString(USART2, "Data verification FAILED!\r\n");

    UART_WriteString(USART2, "All I2C API tests completed.\r\n");

    while(1);
}
