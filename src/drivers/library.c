#include "stm32f103xb.h"
#include "library.h"

#define LED_PIN 5

static bool ledInitilised = false;

// ---------------- GPIO Init ----------------
static void LED_Init(void)
{
    // Enable clock for GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    // Configure PA5 as output push-pull, 2 MHz
    GPIOA->CRL &= ~(0xF << (LED_PIN * 4));
    GPIOA->CRL |= (0x2 << (LED_PIN * 4));
    ledInitilised = true;
}

// ---------------- LED Toggle ----------------
void LED_Toggle(void)
{
    if (!ledInitilised)
        LED_Init();
    GPIOA->ODR ^= (1 << LED_PIN);
}
// ---------------- LED on ----------------
void LED_On(void)
{
    if (!ledInitilised)
        LED_Init();
    GPIOA->ODR |= (1 << LED_PIN);
}
// ---------------- LED on ----------------
void LED_Off(void)
{
    if (!ledInitilised)
        LED_Init();
    GPIOA->ODR &= ~(1 << LED_PIN);
}

