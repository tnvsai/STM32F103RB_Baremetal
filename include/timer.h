#ifndef TIMER_H
#define TIMER_H

#include "stm32f103xb.h"
#include <stdint.h>

// ---------------- Timer IDs ----------------
typedef enum {
    TIMER1,
    TIMER2,
    TIMER3,
    TIMER4
} Timer_Id_t;

// ---------------- Timer callback type ----------------
typedef void (*Timer_Callback_t)(void);

// ---------------- Basic Timer ----------------
void TIMER_InitMs(Timer_Id_t timer, uint32_t ms, Timer_Callback_t callback);
void TIMER_Start(Timer_Id_t timer);
void TIMER_Stop(Timer_Id_t timer);
uint8_t TIMER_HasElapsed(Timer_Id_t timer);
void TIMER_ClearFlag(Timer_Id_t timer);
void TIMER_EnableInterrupt(Timer_Id_t timer);
void TIMER_DisableInterrupt(Timer_Id_t timer);

// ---------------- PWM ----------------
void TIMER_InitPWM(Timer_Id_t timer, uint8_t channel, uint16_t prescaler, uint16_t arr);
void TIMER_SetPWMDuty(Timer_Id_t timer, uint8_t channel, uint16_t duty);

// ---------------- Input Capture ----------------
void TIMER_InitInputCapture(Timer_Id_t timer, uint8_t channel, uint16_t prescaler, uint16_t arr, Timer_Callback_t callback);

#endif
