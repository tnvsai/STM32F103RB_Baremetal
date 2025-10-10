#include "timer.h"
#include "stm32f103xb.h"

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
static Timer_Callback_t ic_callbacks[4]    = {0};

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

// ---------------- PWM Init ----------------
void TIMER_InitPWM(Timer_Id_t timer, uint8_t channel, uint16_t prescaler, uint16_t arr) {
    TIM_TypeDef *TIMx = TIMER_GetBase(timer);
    if (!TIMx) return;

    TIMER_EnableClock(timer);

    // -------- GPIO Configuration --------
    switch(timer) {
        case TIMER1:
            RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
            switch(channel) {
                case 1: GPIOA->CRH = (GPIOA->CRH & ~(0xF << 0)) | (0xB << 0); break; // PA8
                case 2: GPIOA->CRH = (GPIOA->CRH & ~(0xF << 4)) | (0xB << 4); break; // PA9
                case 3: GPIOA->CRH = (GPIOA->CRH & ~(0xF << 8)) | (0xB << 8); break; // PA10
                case 4: GPIOA->CRH = (GPIOA->CRH & ~(0xF << 12)) | (0xB << 12); break; // PA11
            }
            break;
        case TIMER2:
            RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
            switch(channel) {
                case 1: GPIOA->CRL = (GPIOA->CRL & ~(0xF << 0)) | (0xB << 0); break; // PA0
                case 2: GPIOA->CRL = (GPIOA->CRL & ~(0xF << 4)) | (0xB << 4); break; // PA1
                case 3: GPIOA->CRL = (GPIOA->CRL & ~(0xF << 8)) | (0xB << 8); break; // PA2
                case 4: GPIOA->CRL = (GPIOA->CRL & ~(0xF << 12)) | (0xB << 12); break; // PA3
            }
            break;
        case TIMER3:
            RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
            switch(channel) {
                case 1: GPIOA->CRL = (GPIOA->CRL & ~(0xF << 24)) | (0xB << 24); break; // PA6
                case 2: GPIOA->CRL = (GPIOA->CRL & ~(0xF << 28)) | (0xB << 28); break; // PA7
                case 3: GPIOB->CRL = (GPIOB->CRL & ~(0xF << 0))  | (0xB << 0); break;  // PB0
                case 4: GPIOB->CRL = (GPIOB->CRL & ~(0xF << 4))  | (0xB << 4); break;  // PB1
            }
            break;
        case TIMER4:
            RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
            switch(channel) {
                case 1: GPIOB->CRL = (GPIOB->CRL & ~(0xF << 24)) | (0xB << 24); break; // PB6
                case 2: GPIOB->CRL = (GPIOB->CRL & ~(0xF << 28)) | (0xB << 28); break; // PB7
                case 3: GPIOB->CRH = (GPIOB->CRH & ~(0xF << 0))  | (0xB << 0); break;  // PB8
                case 4: GPIOB->CRH = (GPIOB->CRH & ~(0xF << 4))  | (0xB << 4); break;  // PB9
            }
            break;
    }

    // -------- Timer Configuration --------
    TIMx->PSC = prescaler;
    TIMx->ARR = arr;
    TIMx->EGR = TIM_EGR_UG;

    // Configure PWM mode
    switch(channel) {
        case 1: TIMx->CCMR1 = (TIMx->CCMR1 & ~TIM_CCMR1_OC1M) | (6 << TIM_CCMR1_OC1M_Pos); TIMx->CCMR1 |= TIM_CCMR1_OC1PE; TIMx->CCER |= TIM_CCER_CC1E; break;
        case 2: TIMx->CCMR1 = (TIMx->CCMR1 & ~TIM_CCMR1_OC2M) | (6 << TIM_CCMR1_OC2M_Pos); TIMx->CCMR1 |= TIM_CCMR1_OC2PE; TIMx->CCER |= TIM_CCER_CC2E; break;
        case 3: TIMx->CCMR2 = (TIMx->CCMR2 & ~TIM_CCMR2_OC3M) | (6 << TIM_CCMR2_OC3M_Pos); TIMx->CCMR2 |= TIM_CCMR2_OC3PE; TIMx->CCER |= TIM_CCER_CC3E; break;
        case 4: TIMx->CCMR2 = (TIMx->CCMR2 & ~TIM_CCMR2_OC4M) | (6 << TIM_CCMR2_OC4M_Pos); TIMx->CCMR2 |= TIM_CCMR2_OC4PE; TIMx->CCER |= TIM_CCER_CC4E; break;
    }

    TIMx->CR1 |= TIM_CR1_ARPE;
    TIMx->CR1 |= TIM_CR1_CEN;

    if (timer == TIMER1) TIM1->BDTR |= TIM_BDTR_MOE; // TIM1 main output enable
}

void TIMER_SetPWMDuty(Timer_Id_t timer, uint8_t channel, uint16_t duty) {
    TIM_TypeDef *TIMx = TIMER_GetBase(timer);
    if (!TIMx) return;

    switch(channel) {
        case 1: TIMx->CCR1 = duty; break;
        case 2: TIMx->CCR2 = duty; break;
        case 3: TIMx->CCR3 = duty; break;
        case 4: TIMx->CCR4 = duty; break;
    }
}

// ---------------- Input Capture ----------------
void TIMER_InitInputCapture(Timer_Id_t timer, uint8_t channel, uint16_t prescaler, uint16_t arr, Timer_Callback_t callback) {
    TIM_TypeDef *TIMx = TIMER_GetBase(timer);
    if (!TIMx) return;

    TIMER_EnableClock(timer);

    TIMx->PSC = prescaler;
    TIMx->ARR = arr;
    TIMx->CNT = 0;

    ic_callbacks[timer] = callback;

    switch(channel) {
        case 1:
            TIMx->CCMR1 &= ~TIM_CCMR1_CC1S;
            TIMx->CCMR1 |= 1 << 0; // CC1S = 01 → map to TI1
            TIMx->CCER |= TIM_CCER_CC1E; 
            TIMx->DIER |= TIM_DIER_CC1IE;
            break;
        case 2:
            TIMx->CCMR1 &= ~TIM_CCMR1_CC2S;
            TIMx->CCMR1 |= 1 << 8; // CC2S = 01 → map to TI2
            TIMx->CCER |= TIM_CCER_CC2E; 
            TIMx->DIER |= TIM_DIER_CC2IE;
            break;
        case 3:
            TIMx->CCMR2 &= ~TIM_CCMR2_CC3S;
            TIMx->CCMR2 |= 1 << 0; // CC3S = 01 → map to TI3
            TIMx->CCER |= TIM_CCER_CC3E; 
            TIMx->DIER |= TIM_DIER_CC3IE;
            break;
        case 4:
            TIMx->CCMR2 &= ~TIM_CCMR2_CC4S;
            TIMx->CCMR2 |= 1 << 8; // CC4S = 01 → map to TI4
            TIMx->CCER |= TIM_CCER_CC4E; 
            TIMx->DIER |= TIM_DIER_CC4IE;
            break;
    }

    TIMx->CR1 |= TIM_CR1_CEN;

    // Enable NVIC
    switch(timer) {
        case TIMER1: NVIC_EnableIRQ(TIM1_UP_IRQn); break;
        case TIMER2: NVIC_EnableIRQ(TIM2_IRQn); break;
        case TIMER3: NVIC_EnableIRQ(TIM3_IRQn); break;
        case TIMER4: NVIC_EnableIRQ(TIM4_IRQn); break;
    }
}

// Extend existing IRQ handlers for input capture
// #define TIMER_IC_IRQ_HANDLER(IRQ, ID) \
// void IRQ(void) { \
//     TIM_TypeDef *TIMx = TIMER_GetBase(ID); \
//     if ((TIMx->SR & TIM_SR_CC1IF) && ic_callbacks[ID]) { \
//         TIMx->SR &= ~TIM_SR_CC1IF; \
//         ic_callbacks[ID](); \
//     } \
// }

// TIMER_IC_IRQ_HANDLER(TIM1_UP_IRQHandler, TIMER1)
// TIMER_IC_IRQ_HANDLER(TIM2_IRQHandler,   TIMER2)
// TIMER_IC_IRQ_HANDLER(TIM3_IRQHandler,   TIMER3)
// TIMER_IC_IRQ_HANDLER(TIM4_IRQHandler,   TIMER4)
