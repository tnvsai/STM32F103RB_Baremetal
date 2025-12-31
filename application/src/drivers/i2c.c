#include "stm32f103xb.h"
#include "uart.h"

#define I2C_OK      0
#define I2C_ERR     1
#define I2C_TIMEOUT 2

#define I2C_WRITE   0
#define I2C_READ    1

// -----------------------------
// I2C Bus Recovery (toggle SCL 9 times if SDA stuck low)
// -----------------------------
static void I2C_BusRecover(void) {
    GPIOB->CRL &= ~((0xF << 24) | (0xF << 28)); // PB6/PB7 input
    GPIOB->CRL |= ((0x4 << 24) | (0x4 << 28));  // Open-drain input mode

    for (int i=0; i<9; i++) {
        GPIOB->ODR |= (1<<6);
        for (volatile int d=0; d<100; d++);
        GPIOB->ODR &= ~(1<<6);
        for (volatile int d=0; d<100; d++);
    }
}

// -----------------------------
// I2C Initialization with dynamic frequency
// APB1 clock = 8 MHz (adjust if needed)
// freq = desired I2C clock in Hz (100kHz or 400kHz)
// -----------------------------
int I2C_Init(I2C_TypeDef *I2Cx, uint32_t freq) {
    int timeout = 10000;

    // Enable clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // GPIOB
    if (I2Cx == I2C1) RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // Configure PB6=SCL, PB7=SDA AF Open-Drain 2 MHz
    GPIOB->CRL &= ~((0xF << 24) | (0xF << 28));
    GPIOB->CRL |= ((0xA << 24) | (0xA << 28));

    // Reset I2C
    I2Cx->CR1 |= I2C_CR1_SWRST;
    I2Cx->CR1 &= ~I2C_CR1_SWRST;

    // Set peripheral clock frequency in CR2
    uint32_t pclk1 = 8000000; // APB1 = 8 MHz
    I2Cx->CR2 = pclk1 / 1000000; // MHz

    // Calculate CCR for standard mode (<=100 kHz)
    I2Cx->CCR = pclk1 / (freq * 2);
    I2Cx->TRISE = (I2Cx->CR2 & 0x3F) + 1; // Maximum rise time

    // Wait until SDA/SCL idle
    while (((GPIOB->IDR & (1<<6)) == 0 || (GPIOB->IDR & (1<<7)) == 0) && --timeout);
    if (timeout == 0) {
        I2C_BusRecover();
        UART_WriteString(USART2, "I2C bus busy! Bus recovery attempted.\r\n");
    }

    // Enable I2C
    I2Cx->CR1 |= I2C_CR1_PE;

    return I2C_OK;
}

// -----------------------------
// Start / Repeated Start
// -----------------------------
int I2C_Start(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t direction) {
    int timeout;

    I2Cx->CR1 |= I2C_CR1_START;

    timeout = 10000;
    while (!(I2Cx->SR1 & I2C_SR1_SB) && --timeout);
    if (timeout == 0) return I2C_TIMEOUT;

    I2Cx->DR = (direction == I2C_WRITE) ? (addr << 1) : ((addr << 1) | 1);

    timeout = 10000;
    while (!(I2Cx->SR1 & I2C_SR1_ADDR) && --timeout);
    if (timeout == 0) return I2C_TIMEOUT;

    volatile uint32_t temp = I2Cx->SR1 | I2Cx->SR2; // Clear ADDR
    (void)temp;

    return I2C_OK;
}

// -----------------------------
// Write / Read single byte
// -----------------------------
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
    if (I2C_Start(I2Cx, addr, I2C_WRITE) != I2C_OK) return I2C_ERR;

    for (uint16_t i = 0; i < length; i++) {
        if (I2C_Write(I2Cx, data[i]) != I2C_OK) {
            I2C_Stop(I2Cx);
            return I2C_ERR;
        }
    }

    I2C_Stop(I2Cx);
    return I2C_OK;
}

// -----------------------------
// Multi-byte read with repeated start support
// -----------------------------
int I2C_ReadMulti(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *data, uint16_t length) {
    if (I2C_Start(I2Cx, addr, I2C_READ) != I2C_OK) return I2C_ERR;

    for (uint16_t i = 0; i < length; i++) {
        data[i] = I2C_Read(I2Cx, (i < length - 1) ? 1 : 0); // ACK all but last
    }

    I2C_Stop(I2Cx);
    return I2C_OK;
}
