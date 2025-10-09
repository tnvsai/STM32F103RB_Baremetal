#ifndef ADC_H
#define ADC_H

#include "stm32f103xb.h"
#include <stdint.h>

// ADC channels
#define ADC_CHANNEL_0    0   // PA0
#define ADC_CHANNEL_1    1   // PA1
#define ADC_CHANNEL_2    2   // PA2
#define ADC_CHANNEL_3    3   // PA3
#define ADC_CHANNEL_4    4   // PA4
#define ADC_CHANNEL_5    5   // PA5
#define ADC_CHANNEL_6    6   // PA6
#define ADC_CHANNEL_7    7   // PA7
#define ADC_CHANNEL_8    8   // PB0
#define ADC_CHANNEL_9    9   // PB1
#define ADC_CHANNEL_10   10  // PC0
#define ADC_CHANNEL_11   11  // PC1
#define ADC_CHANNEL_12   12  // PC2
#define ADC_CHANNEL_13   13  // PC3
#define ADC_CHANNEL_14   14  // PC4
#define ADC_CHANNEL_15   15  // PC5

// Initialize ADC1 and configure pins as analog input
void ADC_Init(void);

// Read single ADC channel (use defined ADC_CHANNEL_X)
uint16_t ADC_Read_Single(uint8_t channel);

#endif
