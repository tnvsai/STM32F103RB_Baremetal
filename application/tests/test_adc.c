#include "stm32f103xb.h"
#include "adc.h"
#include "uart.h"
#include <stdio.h>  // for snprintf

int main(void) {
    uint16_t adc_value;
    uint16_t millivolts;
    char buffer[50];

    // -----------------------------
    // Initialize UART2
    // -----------------------------
    UART_Config_t uart2_cfg = {
        .baudRate   = 115200,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits   = UART_STOPBITS_1,
        .parity     = UART_PARITY_NONE,
        .enableTx   = 1,
        .enableRx   = 1
    };
    UART_Init(USART2, &uart2_cfg);

    // Stabilize UART
    for (volatile int i = 0; i < 10000; i++);
    UART_WriteString(USART2, "\r\n");
    UART_WriteString(USART2, "USART2 test ready!\r\n");

    // -----------------------------
    // Initialize ADC1
    // -----------------------------
    ADC_Init();

    // -----------------------------
    // Main loop: read potentiometer and send voltage in mV
    // -----------------------------
    while (1) {
        // Read ADC channel 0 (PA0)
        adc_value = ADC_Read_Single(ADC_CHANNEL_0);

        // Convert ADC value to millivolts
        millivolts = (adc_value * 3300) / 4095;

        // Format string safely
        snprintf(buffer, sizeof(buffer), "ADC: %u, Voltage: %u mV\r\n", adc_value, millivolts);

        // Send string over UART2
        UART_WriteString(USART2, buffer);

        // Very short delay to avoid flooding UART
        for (volatile int i = 0; i < 500000; i++);
    }
}
