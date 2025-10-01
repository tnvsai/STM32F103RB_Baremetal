#include "spi.h"

// ------------------- GPIO Init -------------------
void SPI1_GPIO_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;  // Enable GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;  // Enable SPI1 clock

    // PA5-SCK (AF PP 50MHz)
    GPIOA->CRL &= ~(0xF << (5*4));
    GPIOA->CRL |= 0xB << (5*4);

    // PA6-MISO (Floating input)
    GPIOA->CRL &= ~(0xF << (6*4));
    GPIOA->CRL |= 0x4 << (6*4);

    // PA7-MOSI (AF PP 50MHz)
    GPIOA->CRL &= ~(0xF << (7*4));
    GPIOA->CRL |= 0xB << (7*4);
}

void SPI2_GPIO_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;  // Enable GPIOB clock
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;  // Enable SPI2 clock

    // PB13-SCK (AF PP 50MHz)
    GPIOB->CRH &= ~(0xF << ((13-8)*4));
    GPIOB->CRH |= 0xB << ((13-8)*4);

    // PB14-MISO (Floating input)
    GPIOB->CRH &= ~(0xF << ((14-8)*4));
    GPIOB->CRH |= 0x4 << ((14-8)*4);

    // PB15-MOSI (AF PP 50MHz)
    GPIOB->CRH &= ~(0xF << ((15-8)*4));
    GPIOB->CRH |= 0xB << ((15-8)*4);
}

// ------------------- SPI Init -------------------
void SPI1_Init(void) {
    SPI1->CR1 = 0;
    SPI1->CR1 |= SPI_CR1_MSTR;    // Master mode
    SPI1->CR1 |= SPI_CR1_SSM;     // Software slave management
    SPI1->CR1 |= SPI_CR1_SSI;     // Internal slave select
    SPI1->CR1 &= ~SPI_CR1_BR;     // Clear baud rate
    SPI1->CR1 |= (0x2 << 3);      // BR = fPCLK/8 → safe for ADXL345
    SPI1->CR1 |= SPI_CR1_CPOL;    // Clock idle high
    SPI1->CR1 |= SPI_CR1_CPHA;    // Capture on 2nd edge
    SPI1->CR1 |= SPI_CR1_SPE;     // Enable SPI
}


void SPI2_Init(void) {
    SPI2->CR1 = 0;
    SPI2->CR1 |= SPI_CR1_MSTR;
    SPI2->CR1 |= SPI_CR1_SSM;
    SPI2->CR1 |= SPI_CR1_SSI;
    SPI2->CR1 |= SPI_CR1_BR_1;      // fPCLK/8
    SPI2->CR1 |= SPI_CR1_CPOL;
    SPI2->CR1 |= SPI_CR1_CPHA;
    SPI2->CR1 |= SPI_CR1_SPE;
}

// ------------------- Single Byte Transmit/Receive -------------------
uint8_t SPI1_TransmitReceive(uint8_t data) {
    uint32_t timeout = SPI1_TIMEOUT;
    while(!(SPI1->SR & SPI_SR_TXE) && timeout--) {}
    if(timeout == 0) return 0xFF; // timeout

    SPI1->DR = data;

    timeout = SPI1_TIMEOUT;
    while(!(SPI1->SR & SPI_SR_RXNE) && timeout--) {}
    if(timeout == 0) return 0xFF;

    return SPI1->DR;
}

uint8_t SPI2_TransmitReceive(uint8_t data) {
    uint32_t timeout = SPI2_TIMEOUT;
    while(!(SPI2->SR & SPI_SR_TXE) && timeout--) {}
    if(timeout == 0) return 0xFF;

    SPI2->DR = data;

    timeout = SPI2_TIMEOUT;
    while(!(SPI2->SR & SPI_SR_RXNE) && timeout--) {}
    if(timeout == 0) return 0xFF;

    return SPI2->DR;
}

uint8_t SPI1_Transmit(uint8_t data) { return SPI1_TransmitReceive(data); }
uint8_t SPI2_Transmit(uint8_t data) { return SPI2_TransmitReceive(data); }

// ------------------- Multi-byte Transmit/Receive -------------------
uint8_t SPI1_TransmitBuffer(uint8_t *txBuf, uint8_t *rxBuf, uint16_t len) {
    for(uint16_t i=0; i<len; i++) {
        uint8_t rx = SPI1_TransmitReceive(txBuf[i]);
        if(rx == 0xFF) return SPI_TIMEOUT;
        rxBuf[i] = rx;
    }
    return SPI_OK;
}

uint8_t SPI2_TransmitBuffer(uint8_t *txBuf, uint8_t *rxBuf, uint16_t len) {
    for(uint16_t i=0; i<len; i++) {
        uint8_t rx = SPI2_TransmitReceive(txBuf[i]);
        if(rx == 0xFF) return SPI_TIMEOUT;
        rxBuf[i] = rx;
    }
    return SPI_OK;
}
