#include "eeprom.h"
// ATMTC730 24C256N EEPROM
// -----------------------------
// Write single byte
// -----------------------------
int EEPROM_WriteByte(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t data) {
    int ret;

    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_WRITE);
    if(ret != I2C_OK) return ret;

    // Send 16-bit memory address
    I2C_Write(I2Cx, (mem_addr >> 8) & 0xFF); // High byte
    I2C_Write(I2Cx, mem_addr & 0xFF);        // Low byte

    // Send data
    I2C_Write(I2Cx, data);

    I2C_Stop(I2Cx);

    // EEPROM internal write delay (~5ms)
    for(volatile int d=0; d<50000; d++);

    return I2C_OK;
}

// -----------------------------
// Read single byte
// -----------------------------
int EEPROM_ReadByte(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t *data) {
    int ret;

    // Send memory address first
    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_WRITE);
    if(ret != I2C_OK) return ret;

    I2C_Write(I2Cx, (mem_addr >> 8) & 0xFF);
    I2C_Write(I2Cx, mem_addr & 0xFF);
    I2C_Stop(I2Cx);

    // Read byte
    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_READ);
    if(ret != I2C_OK) return ret;

    *data = I2C_Read(I2Cx, 0); // NACK after single byte
    I2C_Stop(I2Cx);

    return I2C_OK;
}

// -----------------------------
// Write multiple bytes
// -----------------------------
int EEPROM_WriteBytes(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t *data, uint16_t length) {
    int ret;

    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_WRITE);
    if(ret != I2C_OK) return ret;

    I2C_Write(I2Cx, (mem_addr >> 8) & 0xFF);
    I2C_Write(I2Cx, mem_addr & 0xFF);

    for(int i=0;i<length;i++) {
        I2C_Write(I2Cx, data[i]);
    }
    I2C_Stop(I2Cx);

    // EEPROM write cycle delay
    for(volatile int d=0; d<50000; d++);

    return I2C_OK;
}

// -----------------------------
// Read multiple bytes
// -----------------------------
int EEPROM_ReadBytes(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t *data, uint16_t length) {
    int ret;

    // Send memory address first
    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_WRITE);
    if(ret != I2C_OK) return ret;

    I2C_Write(I2Cx, (mem_addr >> 8) & 0xFF);
    I2C_Write(I2Cx, mem_addr & 0xFF);
    I2C_Stop(I2Cx);

    // Read bytes
    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_READ);
    if(ret != I2C_OK) return ret;

    for(int i=0;i<length;i++) {
        data[i] = I2C_Read(I2Cx, (i < length-1) ? 1 : 0); // ACK for all but last
    }
    I2C_Stop(I2Cx);

    return I2C_OK;
}
