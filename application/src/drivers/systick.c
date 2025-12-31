#include "systick.h"

static volatile uint32_t systick_ticks = 0;

/**
 * @brief Initialize SysTick for a periodic interrupt
 * @param ticks_per_second: number of ticks per second (e.g., 1000 for 1ms)
 */
void SysTick_Init(uint32_t ticks_per_second)
{
    uint32_t system_clock = 8000000UL; // 8 MHz default internal clock
    uint32_t reload = (system_clock / ticks_per_second) - 1;

    // Ensure reload fits in 24 bits
    if (reload > 0xFFFFFF)
        reload = 0xFFFFFF;

    SysTick->LOAD = reload;            // Set reload value
    SysTick->VAL  = 0;                 // Clear current value

    // Select processor clock (AHB), enable interrupt and counter
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | 
                    SysTick_CTRL_TICKINT_Msk   | 
                    SysTick_CTRL_ENABLE_Msk;
}

/**
 * @brief SysTick interrupt handler — called every tick
 */
void SysTick_Handler(void)
{
    systick_ticks++;
}

/**
 * @brief Get current tick count
 */
uint32_t SysTick_GetTick(void)
{
    return systick_ticks;
}

/**
 * @brief Blocking delay in milliseconds
 */
void SysTick_DelayMs(uint32_t ms)
{
    uint32_t start = systick_ticks;
    while ((systick_ticks - start) < ms);
}
