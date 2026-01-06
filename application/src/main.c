#include "stm32f103xb.h"
#include "common.h"
#include "utility.h"
#include "uart.h"
#include "timer.h"
#include "gpio.h"
#include "rcc.h"

void process_command(uint8_t cmd);

volatile uint8_t uart_rx_command = 0;
volatile uint8_t uart_rx_command_received = 0;

void LED1_Callback(void) 
{ 
     LED_Toggle();
     UART_Log(USART2, "Led toggled\r\n");
}

void USART2_IRQHandler(void)
{
    // Read data
    uart_rx_command = USART2->DR & 0xFF;
    uart_rx_command_received = 1; // set flag
}

void jumptobootloader(void)
{
    TIMER_Stop(TIMER2);
    TIMER_DisableInterrupt(TIMER2);
    
    __disable_irq();
   
    // set flag to diable auto jump from bootloader to application
    SHARED_MEMORY_VAL = SHARED_MAGIC_FLAG;
    SCB->VTOR = ADD_BL_START;
    
    uint32_t bootloader_sp = *((volatile uint32_t *)ADD_BL_START);
    
    uint32_t bootloader_reset_handler = *((volatile uint32_t *)(ADD_BL_START + 4));

    void (*reset_handler)(void) = (void (*)(void))bootloader_reset_handler;
    
    __set_MSP(bootloader_sp);
    
    reset_handler();
    
    while(1);
}

void process_command(uint8_t cmd)
{
    if(CMD_JUMP_BOOT == cmd)
    {
         UART_Log(USART2, "jumping to bootloader\r\n");
        jumptobootloader();
    }
    else
    {
         UART_Log(USART2, "invalid command\r\n");
    }
}

int main(void) {
    
    // UART2 configuration
    const UART_Config_t uart2_cfg = {
        .baudRate   = 230400,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits   = UART_STOPBITS_1,
        .parity     = UART_PARITY_NONE,
        .enableTx   = 1,
        .enableRx   = 1,
        .rxInterrupt = 1,
        .interruptPriority = 10
    };
    // Enable clocks for GPIOC, GPIOA, and AFIO peripherals
    RCC_EnableClock(RCC_APB2, RCC_IOPCEN | RCC_IOPAEN | RCC_AFIOEN);
    
    __enable_irq();

    // Reset the value
    SHARED_MEMORY_VAL = 0x0u;

    TIMER_InitMs(TIMER2, 1000, LED1_Callback);
    TIMER_EnableInterrupt(TIMER2);

    // Initialize UART2 (GPIO pins automatically configured)
    UART_Init(USART2, &uart2_cfg);

    // Send welcome message
     UART_Log(USART2, "we are in application \r\n");
    TIMER_Start(TIMER2);

    while (1) 
    {
        if(uart_rx_command_received)
        {
            uart_rx_command_received = 0;
            process_command(uart_rx_command);
        }
    }
}
