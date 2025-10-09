#ifndef EEPROM_H
#define EEPROM_H

#include "stm32f103xb.h"
#include "i2c.h"

#define EEPROM_ADDR  0x50  // Base I2C address

// Single-byte operations
int EEPROM_WriteByte(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t data);
int EEPROM_ReadByte(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t *data);

// Multi-byte operations
int EEPROM_WriteBytes(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t *data, uint16_t length);
int EEPROM_ReadBytes(I2C_TypeDef *I2Cx, uint16_t mem_addr, uint8_t *data, uint16_t length);

#endif
