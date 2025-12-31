#include "stm32f103xb.h"
#include "timer.h"

// Simple blocking delay (~1 ms per iteration)
void delay_ms(uint32_t ms) {
    for(uint32_t i = 0; i < ms * 8000; i++) __asm__("nop");
}

int main(void) {
    // ---------------- Initialize TIM1_CH1 PWM ----------------
    // Timer: TIMER1, Channel: 1
    // Prescaler = 8-1 → 1 MHz timer tick (1 µs)
    // ARR = 1000-1 → 1 kHz PWM frequency
    TIMER_InitPWM(TIMER1, 1, 8-1, 1000-1);

    // Start with 0% duty
    TIMER_SetPWMDuty(TIMER1, 1, 0);

    // ---------------- Fade LED ----------------
    while (1) {
        // Increase duty cycle 0 → 100%
        for (uint16_t duty = 0; duty <= 1000; duty += 10) {
            TIMER_SetPWMDuty(TIMER1, 1, duty);
            delay_ms(5);
        }

        // Decrease duty cycle 100% → 0%
        for (uint16_t duty = 1000; duty > 0; duty -= 10) {
            TIMER_SetPWMDuty(TIMER1, 1, duty);
            delay_ms(5);
        }
    }
}
