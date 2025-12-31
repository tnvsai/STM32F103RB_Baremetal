#ifndef SPI_H
#define SPI_H

#include "stm32f103xb.h"
#include <stdint.h>

#define SPI_OK        0
#define SPI_ERR       1
#define SPI_TIMEOUT   2

#define SPI1_TIMEOUT  10000
#define SPI2_TIMEOUT  10000

void SPI1_GPIO_Init(void);
void SPI2_GPIO_Init(void);

void SPI1_Init(void);
void SPI2_Init(void);

uint8_t SPI1_Transmit(uint8_t data);
uint8_t SPI2_Transmit(uint8_t data);

uint8_t SPI1_TransmitReceive(uint8_t data);
uint8_t SPI2_TransmitReceive(uint8_t data);

uint8_t SPI1_TransmitBuffer(uint8_t *txBuf, uint8_t *rxBuf, uint16_t len);
uint8_t SPI2_TransmitBuffer(uint8_t *txBuf, uint8_t *rxBuf, uint16_t len);

#endif
