int main(void) {
    // Enable GPIOC + GPIOA clocks
    RCC_EnableClock(RCC_APB2, RCC_IOPCEN | RCC_IOPAEN | RCC_AFIOEN);

    // Configure PC13 as push-pull output
    GPIO_ConfigPin(GPIOA, 5, GPIO_OUTPUT_2M, GPIO_GP_PUSH_PULL);

    // Configure PA0 as input with pull-up
    GPIO_ConfigInput(GPIOC, 13, GPIO_PULLUP);

    while (1) {
        if (GPIO_ReadPin(GPIOC, 13) == 0) { // Button pressed
            GPIO_WritePin(GPIOA, 5, 0);   // LED off
        } else {
            GPIO_WritePin(GPIOA, 5, 1);   // LED on
        }
    }
}