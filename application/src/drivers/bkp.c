#include "bkp.h"

void BKP_Init(void)
{
    // 1. Enable clocks for PWR and BKP
    RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;

    // 2. Enable access to Backup domain (clear write protection)
    PWR->CR |= PWR_CR_DBP;
}

void BKP_WriteReg(BKP_Reg_t reg, uint16_t data)
{
    if (reg < BKP_DR1 || reg > BKP_DR10) return;

    // Each DR register is spaced by 0x04 bytes
    volatile uint32_t *bkp_base = (uint32_t *)(&BKP->DR1);
    bkp_base[reg - 1] = data;
}

uint16_t BKP_ReadReg(BKP_Reg_t reg)
{
    if (reg < BKP_DR1 || reg > BKP_DR10) return 0xFFFF;

    volatile uint32_t *bkp_base = (uint32_t *)(&BKP->DR1);
    return (uint16_t)bkp_base[reg - 1];
}

void BKP_ResetDomain(void)
{
    // This will reset RTC + backup registers
    RCC->BDCR |= RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;
}
