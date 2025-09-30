#include "stm32f103xb.h"
#include "uart.h"

#define I2C_OK      0
#define I2C_ERR     1
#define I2C_TIMEOUT 2

// -----------------------------
// Safe I2C initialization
// -----------------------------
int I2C_Init(I2C_TypeDef *I2Cx, uint32_t freq) {
    int timeout = 10000;

    // 1. Enable clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;     // GPIOB
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;     // I2C1

    // 2. Configure PB6=SCL, PB7=SDA as AF Open-Drain 2 MHz
    GPIOB->CRL &= ~((0xF << 24) | (0xF << 28));
    GPIOB->CRL |=  ((0xA << 24) | (0xA << 28));

    // 3. Reset I2C peripheral
    I2Cx->CR1 |= I2C_CR1_SWRST;
    I2Cx->CR1 &= ~I2C_CR1_SWRST;

    // 4. Timing configuration (APB1 = 8 MHz)
    I2Cx->CR2   = 8;
    I2Cx->CCR   = 40;
    I2Cx->TRISE = 9;

    // 5. Wait for SDA/SCL idle
    while (((GPIOB->IDR & (1<<6)) == 0 || (GPIOB->IDR & (1<<7)) == 0) && --timeout);
    if (timeout == 0) {
        UART_WriteString(USART2, "I2C bus busy! Check SDA/SCL.\r\n");
        return I2C_ERR;
    }

    // 6. Enable I2C
    I2Cx->CR1 |= I2C_CR1_PE;

    // Optional delay
    for (volatile int d=0; d<1000; d++);

    UART_WriteString(USART2, "I2C_Init_Safe completed.\r\n");
    return I2C_OK;
}

// -----------------------------
// Single-byte API (existing)
// -----------------------------
int I2C_Start(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t direction) {
    int timeout;

    I2Cx->CR1 |= I2C_CR1_START;

    timeout = 10000;
    while (!(I2Cx->SR1 & I2C_SR1_SB) && --timeout);
    if (timeout == 0) return I2C_TIMEOUT;

    I2Cx->DR = (direction == 0) ? (addr << 1) : ((addr << 1) | 1);

    timeout = 10000;
    while (!(I2Cx->SR1 & I2C_SR1_ADDR) && --timeout);
    if (timeout == 0) return I2C_TIMEOUT;

    volatile uint32_t temp = I2Cx->SR1 | I2Cx->SR2;
    (void)temp;

    return I2C_OK;
}

int I2C_Write(I2C_TypeDef *I2Cx, uint8_t data) {
    int timeout;

    I2Cx->DR = data;

    timeout = 10000;
    while (!(I2Cx->SR1 & I2C_SR1_TXE) && --timeout);
    if (timeout == 0) return I2C_TIMEOUT;

    timeout = 10000;
    while (!(I2Cx->SR1 & I2C_SR1_BTF) && --timeout);
    if (timeout == 0) return I2C_TIMEOUT;

    return I2C_OK;
}

uint8_t I2C_Read(I2C_TypeDef *I2Cx, uint8_t ack) {
    if (ack) I2Cx->CR1 |= I2C_CR1_ACK;
    else     I2Cx->CR1 &= ~I2C_CR1_ACK;

    int timeout = 10000;
    while (!(I2Cx->SR1 & I2C_SR1_RXNE) && --timeout);
    if (timeout == 0) return 0xFF;

    return I2Cx->DR;
}

void I2C_Stop(I2C_TypeDef *I2Cx) {
    I2Cx->CR1 |= I2C_CR1_STOP;
}

// -----------------------------
// Multi-byte write
// -----------------------------
int I2C_WriteMulti(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *data, uint16_t length) {
    if (I2C_Start(I2Cx, addr, 0) != I2C_OK) return I2C_ERR;

    for (uint16_t i=0; i<length; i++) {
        if (I2C_Write(I2Cx, data[i]) != I2C_OK) {
            I2C_Stop(I2Cx);
            return I2C_ERR;
        }
    }

    I2C_Stop(I2Cx);
    return I2C_OK;
}

// -----------------------------
// Multi-byte read
// -----------------------------
int I2C_ReadMulti(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *data, uint16_t length) {
    if (I2C_Start(I2Cx, addr, 1) != I2C_OK) return I2C_ERR;

    for (uint16_t i=0; i<length; i++) {
        data[i] = I2C_Read(I2Cx, (i < length-1) ? 1 : 0); // ACK all but last
    }

    I2C_Stop(I2Cx);
    return I2C_OK;
}
