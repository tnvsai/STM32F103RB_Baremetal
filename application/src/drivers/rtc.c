#include "stm32f103xb.h"

// -------------------------
// RTC Initialization (LSI)
// -------------------------
void RTC_Init(void)
{
    // 1. Enable backup domain access
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; // enable PWR clock
    PWR->CR |= PWR_CR_DBP;             // enable access
    while (!(PWR->CR & PWR_CR_DBP));

    // 2. Enable LSI
    RCC->CSR |= RCC_CSR_LSION;
    while (!(RCC->CSR & RCC_CSR_LSIRDY));

    // 3. Select LSI as RTC clock
    RCC->BDCR &= ~RCC_BDCR_RTCSEL;
    RCC->BDCR |= RCC_BDCR_RTCSEL_1; // 10 = LSI
    RCC->BDCR |= RCC_BDCR_RTCEN;    // enable RTC

    // 4. Wait for RTC registers to synchronize
    RTC->CRL &= ~RTC_CRL_RSF;
    while (!(RTC->CRL & RTC_CRL_RSF));

    // 5. Enter config mode
    RTC->CRL |= RTC_CRL_CNF;

    // 6. Set prescaler for ~1Hz tick (LSI ~37 kHz)
    RTC->PRLH = (36999 >> 16) & 0xFFFF;
    RTC->PRLL = 36999 & 0xFFFF;

    // 7. Reset counter
    RTC->CNTH = 0;
    RTC->CNTL = 0;

    // 8. Exit config mode
    RTC->CRL &= ~RTC_CRL_CNF;
    while (!(RTC->CRL & RTC_CRL_RTOFF));
}

// -------------------------
// RTC Counter Access
// -------------------------
uint32_t RTC_GetCounter(void)
{
    return ((uint32_t)RTC->CNTH << 16) | RTC->CNTL;
}

void RTC_SetCounter(uint32_t seconds)
{
    RTC->CRL |= RTC_CRL_CNF;
    RTC->CNTH = (seconds >> 16) & 0xFFFF;
    RTC->CNTL = seconds & 0xFFFF;
    RTC->CRL &= ~RTC_CRL_CNF;
    while (!(RTC->CRL & RTC_CRL_RTOFF));
}

// -------------------------
// Convert Counter to HH:MM:SS
// -------------------------
void RTC_GetTime(uint32_t *hours, uint32_t *minutes, uint32_t *seconds)
{
    uint32_t cnt = RTC_GetCounter();
    *hours   = cnt / 3600;
    *minutes = (cnt % 3600) / 60;
    *seconds = cnt % 60;
}

// -------------------------
// RTC Alarm (single)
// -------------------------
void RTC_SetAlarm(uint32_t seconds)
{
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRH = (seconds >> 16) & 0xFFFF;
    RTC->ALRL = seconds & 0xFFFF;
    RTC->CRL &= ~RTC_CRL_CNF;
    while (!(RTC->CRL & RTC_CRL_RTOFF));

    RTC->CRH |= RTC_CRH_ALRIE; // enable alarm interrupt
}

uint8_t RTC_AlarmTriggered(void)
{
    if (RTC->CRL & RTC_CRL_ALRF)   // Correct macro
    {
        RTC->CRL &= ~RTC_CRL_ALRF; // Clear alarm flag
        return 1;
    }
    return 0;
}

