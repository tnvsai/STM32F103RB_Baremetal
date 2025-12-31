int main(void) {
    // UART2 configuration
    UART_Config_t uart2_cfg = {
        .baudRate   = 115200,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits   = UART_STOPBITS_1,
        .parity     = UART_PARITY_NONE,
        .enableTx   = 1,
        .enableRx   = 1
    };

    // Initialize UART2 (GPIO pins automatically configured)
    UART_Init(USART2, &uart2_cfg);

    // Send welcome message
    UART_WriteString(USART2, "USART2 test ready!\r\n");

    while (1) {
        // Send periodic message
        UART_WriteString(USART2, "Hello from STM32F103RB!\r\n");

        // Echo received characters
        if (USART2->SR & (1 << 5)) { // RXNE
            char c = UART_ReadChar(USART2);
            UART_WriteChar(USART2, c); // echo
        }

        // Crude delay ~1 second
        delay(800000);
    }
}