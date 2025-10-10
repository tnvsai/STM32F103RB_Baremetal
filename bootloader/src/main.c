/* bootloader/src/main.c
   Robust bootloader jump routine + LED on (PA5).
*/

#include "stm32f103xb.h"

#define APP_BASE_ADDRESS 0x08004000U   /* Application start (after bootloader) */
#define LED_PIN          5
#define LED_PORT         GPIOA

typedef void (*app_entry_t)(void);

/* --- helpers -------------------------------------------------------------*/

static void LED_InitAndOn(void)
{
    /* Enable GPIOA clock */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    (void)RCC->APB2ENR; /* small delay for clock to take effect (read-back) */

    /* Configure PA5 as output push-pull, max speed 2 MHz */
    LED_PORT->CRL &= ~(0xF << (LED_PIN * 4));
    LED_PORT->CRL |=  (0x2 << (LED_PIN * 4)); /* MODE = 10b (2MHz), CNF = 00 (push-pull) */

    /* Turn LED on */
    LED_PORT->BSRR = (1u << LED_PIN);
}

static void peripheral_deinit(void)
{
    /* Disable SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* Disable all NVIC interrupts and clear pending flags */
    for (int i = 0; i < 8; ++i) {
        NVIC->ICER[i] = 0xFFFFFFFFul;   /* disable */
        NVIC->ICPR[i] = 0xFFFFFFFFul;   /* clear pending */
    }

    /* Disable peripheral clocks (APB1, APB2, AHB) - tidy state */
    RCC->APB1ENR = 0;
    RCC->APB2ENR = 0;
    RCC->AHBENR  = 0;

    /* Optional: reset AFIO mapping if used (not necessary normally) */
    /* AFIO->MAPR = 0; */ 
}

/* Validate the application vector table looks sane:
   - first word should be a RAM pointer in SRAM range (0x20000000..)
   - second word should be an address in Flash (APP_BASE_ADDRESS..FLASH_END)
*/
static int is_application_valid(void)
{
    uint32_t sp = *((uint32_t *)APP_BASE_ADDRESS);
    uint32_t reset = *((uint32_t *)(APP_BASE_ADDRESS + 4));

    /* Check stack pointer: must be in SRAM region */
    if ((sp < 0x20000000U) || (sp > 0x20005000U)) { /* adjust top-of-RAM if needed */
        return 0;
    }

    /* Check reset handler lies in flash (simple check) */
    if ((reset < APP_BASE_ADDRESS) || (reset > 0x0801FFFFU)) { /* adjust FLASH size if different */
        return 0;
    }

    /* Add more checks if desired (e.g. not 0xFFFFFFFF) */
    if (reset == 0xFFFFFFFFU || sp == 0xFFFFFFFFU) return 0;

    return 1;
}

/* The robust jump routine */
static void jump_to_application(void)
{
    uint32_t app_stack  = *((volatile uint32_t *)APP_BASE_ADDRESS);
    uint32_t app_reset  = *((volatile uint32_t *)(APP_BASE_ADDRESS + 4));
    app_entry_t app_entry = (app_entry_t)app_reset;

    /* 1) Disable interrupts globally */
    __disable_irq();

    /* 2) De-init peripherals that could interfere with app startup */
    peripheral_deinit();

    /* 3) Relocate vector table to application base BEFORE enabling interrupts */
    SCB->VTOR = APP_BASE_ADDRESS;

    /* 4) Set MSP to application stack */
    __set_MSP(app_stack);

    /* 5) Re-enable interrupts (optional) — the app will reconfigure NVIC as needed */
    __enable_irq();

    /* 6) Jump to application's Reset_Handler */
    app_entry();
}

/* --- main ----------------------------------------------------------------*/

int main(void)
{
    /* Keep LED ON while in bootloader */
    LED_InitAndOn();

    /* Give a visible delay (so you can observe bootloader LED) */
    for (volatile uint32_t d = 0; d < 3000000U; ++d) __asm("nop");

    /* If the application looks valid, jump; else remain in bootloader (LED stays ON) */
    if (is_application_valid()) {
        jump_to_application();
    }

    /* If we reach here, app is invalid — stay in bootloader with LED on
       (you can implement serial/logging or DFU mode here) */
    while (1) {
        __asm("wfi"); /* low-power wait, LED still lit */
    }
}
