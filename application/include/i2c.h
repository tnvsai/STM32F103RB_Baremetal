#ifndef I2C_H
#define I2C_H

#include "stm32f103xb.h"

// I2C direction
#define I2C_WRITE 0
#define I2C_READ  1

// Return status
#define I2C_OK       0
#define I2C_ERR      1
#define I2C_TIMEOUT  2

// Initialization
int I2C_Init(I2C_TypeDef *I2Cx, uint32_t freq);

// Basic operations
int I2C_Start(I2C_TypeDef *I2Cx, uint8_t address, uint8_t direction);
void I2C_Stop(I2C_TypeDef *I2Cx);
int I2C_Write(I2C_TypeDef *I2Cx, uint8_t data);
uint8_t I2C_Read(I2C_TypeDef *I2Cx, uint8_t ack);

// Multi-byte operations
int I2C_WriteMulti(I2C_TypeDef *I2Cx, uint8_t address, uint8_t *data, uint16_t length);
int I2C_ReadMulti(I2C_TypeDef *I2Cx, uint8_t address, uint8_t *data, uint16_t length);

#endif
