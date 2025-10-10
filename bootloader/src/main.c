/* bootloader/src/main.c
   STM32F103RB Bootloader:
   - Robust jump to application routine
   - LED on PA5
   - UART2 for user interaction
*/

#include "stm32f103xb.h"
#include "uart.h"

#define APP_BASE_ADDRESS 0x08004000U /* Application start address after bootloader */
#define LED_PIN 5
#define LED_PORT GPIOA

typedef void (*app_entry_t)(void);

/* --- Helper Functions ---------------------------------------------------*/

/* Initialize PA5 as output push-pull and turn LED on */
static void LED_InitAndOn(void)
{
    /* Enable GPIOA clock */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    (void)RCC->APB2ENR; /* small delay for clock to take effect */

    /* Configure PA5 as output push-pull, max speed 2 MHz */
    LED_PORT->CRL &= ~(0xF << (LED_PIN * 4));
    LED_PORT->CRL |= (0x2 << (LED_PIN * 4)); /* MODE = 10b (2MHz), CNF = 00 (push-pull) */

    /* Turn LED on */
    LED_PORT->BSRR = (1u << LED_PIN);
}

/* De-initialize peripherals and interrupts before jumping to application */
static void peripheral_deinit(void)
{
    /* Disable SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    /* Disable all NVIC interrupts and clear pending flags */
    for (int i = 0; i < 8; ++i)
    {
        NVIC->ICER[i] = 0xFFFFFFFFul; /* disable */
        NVIC->ICPR[i] = 0xFFFFFFFFul; /* clear pending */
    }

    /* Disable peripheral clocks (APB1, APB2, AHB) */
    RCC->APB1ENR = 0;
    RCC->APB2ENR = 0;
    RCC->AHBENR = 0;
}

/* Validate the application vector table */
static int is_application_valid(void)
{
    uint32_t sp = *((uint32_t *)APP_BASE_ADDRESS);          // initial stack pointer
    uint32_t reset = *((uint32_t *)(APP_BASE_ADDRESS + 4)); // reset handler

    /* Stack pointer must be in SRAM range */
    if ((sp < 0x20000000U) || (sp > 0x20005000U))
        return 0;

    /* Reset handler must lie in flash region of application */
    if ((reset < APP_BASE_ADDRESS) || (reset > 0x0801FFFFU))
        return 0;

    /* Avoid invalid values (erased flash) */
    if (reset == 0xFFFFFFFFU || sp == 0xFFFFFFFFU)
        return 0;

    return 1;
}

/* Jump to application routine */
static void jump_to_application(void)
{
    uint32_t app_stack = *((volatile uint32_t *)APP_BASE_ADDRESS);
    uint32_t app_reset = *((volatile uint32_t *)(APP_BASE_ADDRESS + 4));
    app_entry_t app_entry = (app_entry_t)app_reset;

    /* 1) Disable interrupts globally */
    __disable_irq();

    /* 2) De-init peripherals that could interfere with application */
    peripheral_deinit();

    /* 3) Relocate vector table to application base */
    SCB->VTOR = APP_BASE_ADDRESS;

    /* 4) Set MSP to application stack */
    __set_MSP(app_stack);

    /* 5) Re-enable interrupts */
    __enable_irq();

    /* 6) Jump to application Reset_Handler */
    app_entry();
}

/* --- Main ----------------------------------------------------------------*/

int main(void)
{
    /* 1. Initialize UART2 for communication */
    UART_Config_t uart2_cfg = {
        .baudRate = 115200,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE,
        .enableTx = 1,
        .enableRx = 1
    };
    UART_Init(USART2, &uart2_cfg);

    /* 2. Print bootloader ready message */
    UART_WriteString(USART2, "Bootloader ready!\r\n");
    LED_InitAndOn();
    UART_WriteString(USART2, "Send '5' (ASCII 0x35) to jump to application...\r\n");

    /* 3. Main loop: wait for UART command */
    while (1)
    {
        char cmd = UART_ReadChar(USART2); /* blocking read */

        if (cmd == 0x35) /* '5' ASCII for jump */
        {
            UART_WriteString(USART2, "Jumping to Application!\r\n");
            if (is_application_valid())
                jump_to_application();
            else
                UART_WriteString(USART2, "Invalid Application!\r\n");
        }
        else
        {
            UART_WriteString(USART2, "Unknown Command\r\n");
        }
    }
}
