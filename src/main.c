
#include "stm32f103xb.h"

void delay(volatile uint32_t count) {
    while(count--) {
        __asm("nop");
    }
}

int main(void) {
    // Enable GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    // Configure PA5 as output push-pull, max speed 2 MHz
    GPIOA->CRL &= ~(GPIO_CRL_MODE5 | GPIO_CRL_CNF5);
    GPIOA->CRL |=  (0x2 << GPIO_CRL_MODE5_Pos); // Output mode, 2 MHz
    GPIOA->CRL |=  (0x0 << GPIO_CRL_CNF5_Pos);  // General purpose output push-pull

    while (1) {
        // Toggle PA5
        GPIOA->ODR ^= (1 << 5);
        delay(500000);
    }
}
