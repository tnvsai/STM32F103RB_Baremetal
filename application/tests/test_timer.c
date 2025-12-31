#include "timer.h"
#include "library.h"
#include "gpio.h"
#include "rcc.h"

// Callback function to toggle LED1 (connected to PA5)
void LED1_Callback(void) { 
    LED_Toggle(); 
}

// Callback function to toggle LED2 (connected to PC11)
void LED2_Callback(void) { 
    GPIO_TogglePin(GPIOC, 11); 
}

int main(void) {

    // Enable clocks for GPIOC, GPIOA, and AFIO peripherals
    RCC_EnableClock(RCC_APB2, RCC_IOPCEN | RCC_IOPAEN | RCC_AFIOEN);

    // Configure PC11 as a 2 MHz push-pull output (for LED2)
    GPIO_ConfigPin(GPIOC, 11, GPIO_OUTPUT_2M, GPIO_GP_PUSH_PULL);

    // Initialize TIM2 to trigger LED1_Callback every 1000 ms (1 second)
    TIMER_InitMs(TIMER2, 1000, LED1_Callback);
    TIMER_EnableInterrupt(TIMER2);
    TIMER_Start(TIMER2);

    // Initialize TIM3 to trigger LED2_Callback every 2000 ms (2 seconds)
    TIMER_InitMs(TIMER3, 2000, LED2_Callback);
    TIMER_EnableInterrupt(TIMER3);
    TIMER_Start(TIMER3);

    // Main loop remains free for other tasks
    while(1) {
        // Application code can be placed here
    }
}
