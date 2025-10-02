#include "stm32f103xb.h"
#include "timer.h"
#include "uart.h"
#include "gpio.h"
#include "rcc.h"
#include <stdio.h>
#include "utility.h"

// ------------------------ Global variables ------------------------
volatile uint32_t ic_rising = 0;
volatile uint32_t ic_falling = 0;
volatile uint32_t high_ticks = 0;
volatile uint32_t period_ticks = 0;

volatile float pwm_frequency = 0.0f;
volatile float pwm_duty = 0.0f;

#define MCU_CLOCK 8000000UL  // 8 MHz

// ------------------------ Input Capture Callback ------------------------
void TIM3_IC_Callback(void) {
    static uint8_t edge = 0;
    uint32_t ccr = TIM3->CCR1;

    if (edge == 0) {
        ic_rising = ccr;
        TIM3->CCER |= TIM_CCER_CC1P; // switch to falling edge
        edge = 1;
    } else {
        ic_falling = ccr;
        TIM3->CCER &= ~TIM_CCER_CC1P; // switch to rising edge
        edge = 0;

        // Compute high ticks
        if (ic_falling >= ic_rising)
            high_ticks = ic_falling - ic_rising;
        else
            high_ticks = (TIM3->ARR + 1) - ic_rising + ic_falling;

        // Compute period
        static uint32_t last_rising = 0;
        uint32_t current_rising = ic_rising;
        if (current_rising >= last_rising)
            period_ticks = current_rising - last_rising;
        else
            period_ticks = (TIM3->ARR + 1) - last_rising + current_rising;
        last_rising = current_rising;

        pwm_frequency = (float)MCU_CLOCK / (float)period_ticks;
        pwm_duty = ((float)high_ticks / (float)period_ticks) * 100.0f;
    }
}

// ------------------------ TIM2 PWM Init ------------------------
void TIM2_PWM_Init(void) {
    // TIM2 CH1: prescaler = 8-1, ARR = 1000-1 → 1 kHz
    TIMER_InitPWM(TIMER2, 1, 8-1, 1000-1);
    TIMER_SetPWMDuty(TIMER2, 1, 500); // 50% duty
}

// ------------------------ TIM3 Input Capture Init ------------------------
void TIM3_IC_Init(void) {
    // TIM3 CH1: capture TIM2 PWM
    TIMER_InitInputCapture(TIMER3, 1, 1-1, 0xFFFF, TIM3_IC_Callback);
}

// ------------------------ Main ------------------------
int main(void) {
    char buf[64];

    // Enable GPIO clocks (for LED/button)
    RCC_EnableClock(RCC_APB2, RCC_IOPAEN | RCC_IOPCEN | RCC_AFIOEN);

    // Configure PC13 as input with pull-up (button)
    GPIO_ConfigInput(GPIOC, 13, GPIO_PULLUP);

    // UART2 setup
    UART_Config_t uart2_cfg = {
        .baudRate   = 115200,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits   = UART_STOPBITS_1,
        .parity     = UART_PARITY_NONE,
        .enableTx   = 1,
        .enableRx   = 1
    };
    UART_Init(USART2, &uart2_cfg);
    UART_WriteString(USART2, "USART2 test ready!\r\n");

    // Init PWM and Input Capture
    TIM2_PWM_Init();
    TIM3_IC_Init();

    uint16_t duty_cycle = 500;

    while (1) {
        snprintf(buf, sizeof(buf), "Freq: %.1f Hz, Duty: %.1f %%\r\n", pwm_frequency, pwm_duty);
        UART_WriteString(USART2, buf);

        for (volatile int i=0; i<500000; i++); // simple delay

        // Adjust PWM duty with button
        if (GPIO_ReadPin(GPIOC, 13) == 0) { // pressed
            duty_cycle += 100;
            if (duty_cycle > 1000) duty_cycle = 0;
            TIMER_SetPWMDuty(TIMER2, 1, duty_cycle);
            LED_Toggle(); // toggle LED
        }
    }
}
