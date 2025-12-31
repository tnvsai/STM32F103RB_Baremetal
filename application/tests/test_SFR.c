#include "stm32f103xb.h"
#include "uart.h"
#include <stdint.h>

//---------------------------
// Helper to read SP
//---------------------------
static inline uint32_t read_sp(void) {
    uint32_t sp;
    __asm volatile("mov %0, sp" : "=r"(sp));
    return sp;
}

//---------------------------
// Crude delay
//---------------------------
static void delay(volatile uint32_t count) {
    while(count--) __asm__("nop");
}

//---------------------------
// Nested Functions
//---------------------------
void funcC(void) {
    uint32_t sp = read_sp();
    mini_printf("Entered funcC: SP = 0x%X\r\n", sp);
    delay(50000);
    mini_printf("Returning from funcC: SP = 0x%X\r\n", sp);
}

void funcB(void) {
    uint32_t sp = read_sp();
    mini_printf("Entered funcB: SP = 0x%X\r\n", sp);
    delay(50000);

    funcC();  // Nested call

    mini_printf("Returning from funcB: SP = 0x%X\r\n", sp);
}

void funcA(void) {
    uint32_t sp = read_sp();
    mini_printf("Entered funcA: SP = 0x%X\r\n", sp);
    delay(50000);

    funcB();  // Nested call

    mini_printf("Returning from funcA: SP = 0x%X\r\n", sp);
}

//---------------------------
// Main Function
//---------------------------
int main(void) {
    // UART2 Configuration
    UART_Config_t uart2_cfg = {
        .baudRate = 115200,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE,
        .enableTx = 1,
        .enableRx = 0
    };
    UART_Init(USART2, &uart2_cfg);

    mini_printf("\r\n=== Nested Function Stack Demo ===\r\n");

    uint32_t sp_main = read_sp();
    mini_printf("Main start: SP = 0x%X\r\n", sp_main);

    funcA(); // Start nested calls

    uint32_t sp_end = read_sp();
    mini_printf("Main end: SP = 0x%X\r\n", sp_end);

    mini_printf("=== Demo Complete ===\r\n");

    while(1);
}
