#include "gpio.h"

// Configure pin (generic)
void GPIO_ConfigPin(GPIO_TypeDef *GPIOx, uint8_t pin, GPIO_ModeSpeed_t mode, GPIO_Config_t config) {
    uint32_t shift = (pin % 8) * 4;
    volatile uint32_t *reg = (pin < 8) ? &GPIOx->CRL : &GPIOx->CRH;

    *reg &= ~(0xF << shift);  // Clear bits
    *reg |= ((mode & 0x3) | ((config & 0x3) << 2)) << shift;
}

// Configure input with pull-up/pull-down
void GPIO_ConfigInput(GPIO_TypeDef *GPIOx, uint8_t pin, GPIO_Pull_t pull) {
    GPIO_ConfigPin(GPIOx, pin, GPIO_INPUT, GPIO_PU_PD);

    if (pull == GPIO_PULLUP) {
        GPIOx->ODR |= (1 << pin);   // Enable pull-up
    } else if (pull == GPIO_PULLDOWN) {
        GPIOx->ODR &= ~(1 << pin);  // Enable pull-down
    }
}

// Write pin
void GPIO_WritePin(GPIO_TypeDef *GPIOx, uint8_t pin, uint8_t value) {
    if (value)
        GPIOx->BSRR = (1 << pin);
    else
        GPIOx->BRR = (1 << pin);
}

// Toggle pin
void GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint8_t pin) {
    GPIOx->ODR ^= (1 << pin);
}

// Read pin
uint8_t GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint8_t pin) {
    return (GPIOx->IDR >> pin) & 0x1;
}
