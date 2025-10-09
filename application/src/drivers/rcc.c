#include "rcc.h"

// Enable peripheral clock
void RCC_EnableClock(RCC_Bus_t bus, uint32_t peripheral) {
    switch(bus) {
        case RCC_AHB:
            RCC->AHBENR |= peripheral;
            break;
        case RCC_APB1:
            RCC->APB1ENR |= peripheral;
            break;
        case RCC_APB2:
            RCC->APB2ENR |= peripheral;
            break;
    }
}

// Disable peripheral clock
void RCC_DisableClock(RCC_Bus_t bus, uint32_t peripheral) {
    switch(bus) {
        case RCC_AHB:
            RCC->AHBENR &= ~peripheral;
            break;
        case RCC_APB1:
            RCC->APB1ENR &= ~peripheral;
            break;
        case RCC_APB2:
            RCC->APB2ENR &= ~peripheral;
            break;
    }
}
