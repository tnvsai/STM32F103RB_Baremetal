#ifndef RCC_H
#define RCC_H

#include <stdint.h>
#include "stm32f103xb.h"

// Bus identifiers
typedef enum {
    RCC_AHB,
    RCC_APB1,
    RCC_APB2
} RCC_Bus_t;

// APB2 peripheral masks (common ones, can extend later)
#define RCC_IOPAEN   (1 << 2)   // GPIOA clock enable
#define RCC_IOPBEN   (1 << 3)   // GPIOB clock enable
#define RCC_IOPCEN   (1 << 4)   // GPIOC clock enable
#define RCC_AFIOEN   (1 << 0)   // AFIO clock enable

// Function prototypes
void RCC_EnableClock(RCC_Bus_t bus, uint32_t peripheral);
void RCC_DisableClock(RCC_Bus_t bus, uint32_t peripheral);

#endif // RCC_H
