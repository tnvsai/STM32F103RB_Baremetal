#ifndef TIMER_H
#define TIMER_H

#include "stm32f103xb.h"

// Timer IDs
typedef enum {
    TIMER1,
    TIMER2,
    TIMER3,
    TIMER4
} Timer_Id_t;

// Timer callback type
typedef void (*Timer_Callback_t)(void);

// Initialize timer in milliseconds with callback
void TIMER_InitMs(Timer_Id_t timer, uint32_t ms, Timer_Callback_t callback);

// Start/Stop timer
void TIMER_Start(Timer_Id_t timer);
void TIMER_Stop(Timer_Id_t timer);

// Enable/Disable interrupts
void TIMER_EnableInterrupt(Timer_Id_t timer);
void TIMER_DisableInterrupt(Timer_Id_t timer);

// Polling mode (optional)
uint8_t TIMER_HasElapsed(Timer_Id_t timer);
void TIMER_ClearFlag(Timer_Id_t timer);

#endif
