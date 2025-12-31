#include "eeprom.h"

// EEPROM constants
#define EEPROM_ADDR       0x50        // 7-bit I2C address
#define EEPROM_PAGE_SIZE  64          // EEPROM page size in bytes
#define EEPROM_WRITE_DELAY 5000       // Minimal software delay if needed (us)

// -----------------------------
// Wait until EEPROM is ready (ACK polling)
// -----------------------------
static int EEPROM_WaitReady(I2C_TypeDef *I2Cx) {
    int timeout = 10000;
    while(timeout--) {
        if(I2C_Start(I2Cx, EEPROM_ADDR, I2C_WRITE) == I2C_OK) {
            I2C_Stop(I2Cx);
            return I2C_OK; // EEPROM ready
        }
    }
    return I2C_TIMEOUT; // Timeout waiting for ACK
}

// -----------------------------
// Write single byte
// -----------------------------
int EEPROM_WriteByte(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t data) {
    int ret;

    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_WRITE);
    if(ret != I2C_OK) return ret;

    // Send 16-bit memory address
    if(I2C_Write(I2Cx, (mem_addr >> 8) & 0xFF) != I2C_OK) { I2C_Stop(I2Cx); return I2C_ERR; }
    if(I2C_Write(I2Cx, mem_addr & 0xFF) != I2C_OK) { I2C_Stop(I2Cx); return I2C_ERR; }

    // Send data byte
    if(I2C_Write(I2Cx, data) != I2C_OK) { I2C_Stop(I2Cx); return I2C_ERR; }

    I2C_Stop(I2Cx);

    // Wait for write completion using ACK polling
    return EEPROM_WaitReady(I2Cx);
}

// -----------------------------
// Read single byte
// -----------------------------
int EEPROM_ReadByte(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t *data) {
    int ret;

    // Set memory address (write)
    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_WRITE);
    if(ret != I2C_OK) return ret;

    if(I2C_Write(I2Cx, (mem_addr >> 8) & 0xFF) != I2C_OK) { I2C_Stop(I2Cx); return I2C_ERR; }
    if(I2C_Write(I2Cx, mem_addr & 0xFF) != I2C_OK) { I2C_Stop(I2Cx); return I2C_ERR; }

    // Repeated START for read
    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_READ);
    if(ret != I2C_OK) return ret;

    *data = I2C_Read(I2Cx, 0); // NACK after single byte
    I2C_Stop(I2Cx);

    return I2C_OK;
}

// -----------------------------
// Write multiple bytes (page-aware)
// -----------------------------
int EEPROM_WriteBytes(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t *data, uint16_t length) {
    int ret = I2C_OK;
    uint16_t remaining = length;
    uint16_t offset = 0;

    while(remaining > 0) {
        uint16_t page_start = mem_addr % EEPROM_PAGE_SIZE;
        uint16_t space_in_page = EEPROM_PAGE_SIZE - page_start;
        uint16_t to_write = (remaining < space_in_page) ? remaining : space_in_page;

        // Start write sequence
        ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_WRITE);
        if(ret != I2C_OK) return ret;

        // Send memory address
        if(I2C_Write(I2Cx, (mem_addr >> 8) & 0xFF) != I2C_OK) { I2C_Stop(I2Cx); return I2C_ERR; }
        if(I2C_Write(I2Cx, mem_addr & 0xFF) != I2C_OK) { I2C_Stop(I2Cx); return I2C_ERR; }

        // Send bytes for this page
        for(uint16_t i=0; i<to_write; i++) {
            if(I2C_Write(I2Cx, data[offset + i]) != I2C_OK) {
                I2C_Stop(I2Cx);
                return I2C_ERR;
            }
        }
        I2C_Stop(I2Cx);

        // Wait until EEPROM write completes
        if((ret = EEPROM_WaitReady(I2Cx)) != I2C_OK) return ret;

        // Advance pointers
        mem_addr += to_write;
        offset += to_write;
        remaining -= to_write;
    }

    return I2C_OK;
}

// -----------------------------
// Read multiple bytes
// -----------------------------
int EEPROM_ReadBytes(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t *data, uint16_t length) {
    int ret;

    // Set memory address (write)
    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_WRITE);
    if(ret != I2C_OK) return ret;

    if(I2C_Write(I2Cx, (mem_addr >> 8) & 0xFF) != I2C_OK) { I2C_Stop(I2Cx); return I2C_ERR; }
    if(I2C_Write(I2Cx, mem_addr & 0xFF) != I2C_OK) { I2C_Stop(I2Cx); return I2C_ERR; }

    // Repeated START for read
    ret = I2C_Start(I2Cx, EEPROM_ADDR, I2C_READ);
    if(ret != I2C_OK) return ret;

    for(uint16_t i=0; i<length; i++) {
        data[i] = I2C_Read(I2Cx, (i < length-1) ? 1 : 0); // ACK all but last byte
    }
    I2C_Stop(I2Cx);

    return I2C_OK;
}
