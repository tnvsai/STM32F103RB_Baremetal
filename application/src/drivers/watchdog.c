#include "watchdog.h"

#define LSI_FREQ 40000UL   // Approximate LSI frequency
#define WWDG_PRESCALER 1   // WWDG prescaler

// ---------------- IWDG ----------------
void IWDG_Init(uint16_t timeout_ms) {
    IWDG->KR = 0x5555; // Unlock
    IWDG->PR = 0x04;   // Prescaler /64

    uint32_t reload = ((timeout_ms * (LSI_FREQ / 64)) / 1000) - 1;
    if(reload > 0x0FFF) reload = 0x0FFF;
    IWDG->RLR = (uint16_t)reload;

    IWDG->KR = 0xAAAA; // Reload
    IWDG->KR = 0xCCCC; // Start
}

void IWDG_Refresh(void) {
    IWDG->KR = 0xAAAA; // Refresh
}

// ---------------- WWDG ----------------
void WWDG_Init(uint8_t window) {
    RCC->APB1ENR |= (1 << 11);           // Enable WWDG clock
    WWDG->CFR = ((window & 0x7F) | (0 << 7)); // prescaler /1, window
    WWDG->CR = 0x7F;                     // Start counter
}

void WWDG_Refresh(void) {
    WWDG->CR = 0x7F; // Refresh within window
}
