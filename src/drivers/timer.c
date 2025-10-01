#include "timer.h"

#define MCU_CLOCK 8000000UL // 8 MHz

// ---------------- Helper: get TIM base ----------------
static TIM_TypeDef* TIMER_GetBase(Timer_Id_t timer) {
    switch(timer) {
        case TIMER1: return TIM1;
        case TIMER2: return TIM2;
        case TIMER3: return TIM3;
        case TIMER4: return TIM4;
        default: return 0;
    }
}

// ---------------- Helper: enable TIM clock ----------------
static void TIMER_EnableClock(Timer_Id_t timer) {
    switch(timer) {
        case TIMER1: RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; break;
        case TIMER2: RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; break;
        case TIMER3: RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; break;
        case TIMER4: RCC->APB1ENR |= RCC_APB1ENR_TIM4EN; break;
    }
}

// ---------------- Callback storage ----------------
static Timer_Callback_t timer_callbacks[4] = {0};

// ---------------- Initialize timer in milliseconds ----------------
void TIMER_InitMs(Timer_Id_t timer, uint32_t ms, Timer_Callback_t callback) {
    TIM_TypeDef *TIMx = TIMER_GetBase(timer);
    if (!TIMx) return;

    TIMER_EnableClock(timer);
    timer_callbacks[timer] = callback;

    // Prescaler for 1 kHz tick → 1 ms
    uint32_t prescaler = (MCU_CLOCK / 1000) - 1;
    uint32_t arr = ms - 1;

    TIMx->PSC = prescaler;
    TIMx->ARR = arr;
    TIMx->CNT = 0;
    TIMx->SR  = 0; // clear UIF flag
}

// ---------------- Start/Stop ----------------
void TIMER_Start(Timer_Id_t timer) {
    TIM_TypeDef *TIMx = TIMER_GetBase(timer);
    if (TIMx) TIMx->CR1 |= TIM_CR1_CEN;
}

void TIMER_Stop(Timer_Id_t timer) {
    TIM_TypeDef *TIMx = TIMER_GetBase(timer);
    if (TIMx) TIMx->CR1 &= ~TIM_CR1_CEN;
}

// ---------------- Polling Mode ----------------
uint8_t TIMER_HasElapsed(Timer_Id_t timer) {
    TIM_TypeDef *TIMx = TIMER_GetBase(timer);
    if (!TIMx) return 0;

    if (TIMx->SR & TIM_SR_UIF) {
        TIMx->SR &= ~TIM_SR_UIF;  // clear flag
        return 1;
    }
    return 0;
}

void TIMER_ClearFlag(Timer_Id_t timer) {
    TIM_TypeDef *TIMx = TIMER_GetBase(timer);
    if (TIMx) TIMx->SR &= ~TIM_SR_UIF;
}

// ---------------- Interrupt Mode ----------------
void TIMER_EnableInterrupt(Timer_Id_t timer) {
    TIM_TypeDef *TIMx = TIMER_GetBase(timer);
    if (!TIMx) return;

    TIMx->DIER |= TIM_DIER_UIE; // enable update interrupt

    switch(timer) {
        case TIMER1: NVIC_EnableIRQ(TIM1_UP_IRQn); break;
        case TIMER2: NVIC_EnableIRQ(TIM2_IRQn); break;
        case TIMER3: NVIC_EnableIRQ(TIM3_IRQn); break;
        case TIMER4: NVIC_EnableIRQ(TIM4_IRQn); break;
    }
}

void TIMER_DisableInterrupt(Timer_Id_t timer) {
    TIM_TypeDef *TIMx = TIMER_GetBase(timer);
    if (!TIMx) return;

    TIMx->DIER &= ~TIM_DIER_UIE;

    switch(timer) {
        case TIMER1: NVIC_DisableIRQ(TIM1_UP_IRQn); break;
        case TIMER2: NVIC_DisableIRQ(TIM2_IRQn); break;
        case TIMER3: NVIC_DisableIRQ(TIM3_IRQn); break;
        case TIMER4: NVIC_DisableIRQ(TIM4_IRQn); break;
    }
}

// ---------------- Timer IRQ handlers ----------------
#define TIMER_IRQ_HANDLER(IRQ, ID) \
void IRQ(void) { \
    TIM_TypeDef *TIMx = TIMER_GetBase(ID); \
    if (TIMx->SR & TIM_SR_UIF) { \
        TIMx->SR &= ~TIM_SR_UIF; \
        if (timer_callbacks[ID]) timer_callbacks[ID](); \
    } \
}

TIMER_IRQ_HANDLER(TIM1_UP_IRQHandler, TIMER1)
TIMER_IRQ_HANDLER(TIM2_IRQHandler,   TIMER2)
TIMER_IRQ_HANDLER(TIM3_IRQHandler,   TIMER3)
TIMER_IRQ_HANDLER(TIM4_IRQHandler,   TIMER4)
