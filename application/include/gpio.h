#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include "stm32f103xb.h"

// Modes
typedef enum {
    GPIO_INPUT      = 0x0,
    GPIO_OUTPUT_10M = 0x1,
    GPIO_OUTPUT_2M  = 0x2,
    GPIO_OUTPUT_50M = 0x3
} GPIO_ModeSpeed_t;

// Config
typedef enum {
    GPIO_ANALOG        = 0x0,
    GPIO_FLOATING      = 0x1,
    GPIO_PU_PD         = 0x2,
    GPIO_GP_PUSH_PULL  = 0x0,
    GPIO_GP_OPEN_DRAIN = 0x1,
    GPIO_AF_PUSH_PULL  = 0x2,
    GPIO_AF_OPEN_DRAIN = 0x3
} GPIO_Config_t;

// Pull type
typedef enum {
    GPIO_NOPULL = 0,
    GPIO_PULLUP,
    GPIO_PULLDOWN
} GPIO_Pull_t;

// API
void GPIO_ConfigPin(GPIO_TypeDef *GPIOx, uint8_t pin, GPIO_ModeSpeed_t mode, GPIO_Config_t config);
void GPIO_ConfigInput(GPIO_TypeDef *GPIOx, uint8_t pin, GPIO_Pull_t pull);
void GPIO_WritePin(GPIO_TypeDef *GPIOx, uint8_t pin, uint8_t value);
void GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint8_t pin);
uint8_t GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint8_t pin);

#endif // GPIO_H
