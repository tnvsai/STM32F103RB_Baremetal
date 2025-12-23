#include "stm32f103xb.h"
#include "uart.h"
#include "flash.h"

// Set to 0 for Production (Host Script), 1 for Manual Debug
// Note: We are now using a prefix so we can leave logging ON.
#define BL_DEBUG 1 

#define LOG_PREFIX "[LOG] "

// --- Commands ---
#define BL_CMD_GET_HELP      0x50
#define BL_CMD_GET_VER       0x51
#define BL_CMD_GET_CID       0x53 
#define BL_CMD_GO            0x55
#define BL_CMD_ERASE_APP     0x56
#define BL_CMD_WRITE_MEM     0x57
#define BL_CMD_READ_MEM      0x59

#define BL_VERSION           0x10

// --- Prototypes ---
void Bootloader_GPIO_Init(void);
void Bootloader_JumpToUserApp(void);
void Bootloader_ProcessCommand(uint8_t cmd);

// Helper for consistent logging
void UART_Log(USART_TypeDef *USARTx, const char *msg) {
    UART_WriteString(USARTx, LOG_PREFIX);
    UART_WriteString(USARTx, msg);
}

int main(void)
{
    // 1. Initialize Hardware
    Bootloader_GPIO_Init();
    
    UART_Config_t uart2_cfg = {
        .baudRate   = 115200,
        .wordLength = UART_WORDLENGTH_8B,
        .stopBits   = UART_STOPBITS_1,
        .parity     = UART_PARITY_NONE,
        .enableTx   = 1,
        .enableRx   = 1
    };
    UART_Init(USART2, &uart2_cfg);


    // 2. Check User Button (PC13)
    // PC13 Pressed = 0
    if (GPIOC->IDR & (1 << 13)) {
        // Button NOT pressed (High) -> Jump to App
        UART_Log(USART2, "Jumping to App...\r\n");
        Bootloader_JumpToUserApp();
    }
    
    UART_Log(USART2, "Bootloader Active v1.0\r\n");
    UART_Log(USART2, "Waiting for commands... 1=Ver, 2=Help, 3=CID, 4=Go, 5=Erase\r\n");

    while (1) {

        uint8_t cmd = (uint8_t)UART_ReadChar(USART2);
        Bootloader_ProcessCommand(cmd);
    }
}

void Bootloader_GPIO_Init(void) {
    // Enable GPIOC Clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    // Configure PC13 as Input with Pull-up/Pull-down
    // CRH register for pin 13 (bits 20-23)
    // Mode = 00 (Input)
    // CNF = 10 (Input with pull-up/pull-down)
    GPIOC->CRH &= ~(0xF << 20); 
    GPIOC->CRH |=  (0x8 << 20);

    // Set ODR to 1 for Pull-Up (assuming active low button)
    GPIOC->ODR |= (1 << 13);

    // Enable GPIOA for LED (PA5) - only when in bootloader mode
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    GPIOA->CRL &= ~(0xF << 20);
    GPIOA->CRL |=  (0x2 << 20);
    GPIOA->ODR |= (1 << 5); // Turn ON LED
}

void Bootloader_JumpToUserApp(void) {
    // Application Address
    uint32_t app_addr = FLASH_START_ADDRESS;

    // 1. Read MSP from the first word of the application
    uint32_t msp_value = *((volatile uint32_t*)app_addr);

    // Check if MSP is valid (in RAM range) - Optional but good safety
    // For STM32F103RB, RAM starts at 0x20000000
    if ((msp_value & 0x2FF00000) == 0x20000000) {
        // 2. Set Main Stack Pointer
        __set_MSP(msp_value);

        // 3. Get Reset Handler Address (second word)
        uint32_t reset_handler_addr = *((volatile uint32_t*)(app_addr + 4));
        void (*app_reset_handler)(void) = (void*)reset_handler_addr;

        // 4. De-init peripherals (UART, GPIO) - Simplified here, just jump
        // Ideally should reset RCC, etc., but often not strictly required if app inits them.
        
        // 5. Jump
        app_reset_handler();
    } else {
        UART_Log(USART2, "Invalid App MSP: 0x");
        UART_WriteHex8(USART2, (msp_value >> 24) & 0xFF);
        UART_WriteHex8(USART2, (msp_value >> 16) & 0xFF);
        UART_WriteHex8(USART2, (msp_value >> 8) & 0xFF);
        UART_WriteHex8(USART2, msp_value & 0xFF);
        UART_WriteString(USART2, "\r\n");
    }
}

void Bootloader_ProcessCommand(uint8_t cmd) {
    uint8_t len;
    uint8_t buffer[64]; 
    uint32_t addr;
    
    // Map ASCII to Commands for manual testing

    if (cmd == '1') cmd = BL_CMD_GET_VER;
    else if (cmd == '2') cmd = BL_CMD_GET_HELP;
    else if (cmd == '4') cmd = BL_CMD_GO;
    else if (cmd == '5') cmd = BL_CMD_ERASE_APP;


    switch (cmd) {
        case BL_CMD_GET_VER:
          UART_Log(USART2, "CMD: Get Version\r\n");
            UART_WriteHex8(USART2, BL_VERSION);
            UART_WriteString(USART2, "\r\n"); 
            break;
        case BL_CMD_GET_HELP:
            UART_Log(USART2, "Help: 1=Ver, 2=Help, 3=CID, 4=Go, 5=Erase\r\n");
            break;
            
        case BL_CMD_ERASE_APP:
            UART_Log(USART2, "Erasing...\r\n");
            if (Flash_EraseAppRegion() == FLASH_OK) {
                UART_Log(USART2, "Result: OK\r\n");
                UART_WriteChar(USART2, 0x06); // ACK
            } else {
                UART_Log(USART2, "Result: FAIL\r\n");
                UART_WriteChar(USART2, 0x15); // NACK
            }
            break;
            
        case BL_CMD_WRITE_MEM:
            // UART_Log(USART2, "WriteMem...\r\n");
            // UNCOMMENT FOR NEW PROTOCOL V2
            UART_WriteChar(USART2, 0x06); // ACK CMD
            
            // Protocol: [ADDR 4B] -> ACK -> [LEN 1B] -> ACK -> [DATA...] -> ACK/NACK
            UART_ReadBuffer(USART2, (uint8_t*)&addr, 4);
            UART_WriteChar(USART2, 0x06); // ACK ADDR

            len = (uint8_t)UART_ReadChar(USART2);
            UART_WriteChar(USART2, 0x06); // ACK LEN

            // Safety check for length to avoid buffer overflow
            if (len > 64) len = 64;

            UART_ReadBuffer(USART2, buffer, len);
            
            uint8_t status = 0x06; // ACK
            
            // Unlock Once
            Flash_Unlock();
            // Clear flags once
            FLASH->SR |= (FLASH_SR_PGERR | FLASH_SR_WRPRTERR);

            // NO interrupt disable - let system breathe
            for (int i = 0; i < len; i += 2) {
                uint16_t data = buffer[i] | (buffer[i+1] << 8);

                // Toggle LED to show liveness
                if((i%3) == 0) GPIOA->ODR ^= (1 << 5); 

                Flash_Status_t f_status = Flash_ProgramHalfWord(addr + i, data);
                if (f_status != FLASH_OK) {
                    status = (uint8_t)f_status; 
                    break;
                }
                
                // Small delay to let Flash controller settle
                for(volatile int d = 0; d < 1000; d++);
            }
            
            // Lock Once
            Flash_Lock();
            
            UART_WriteChar(USART2, status);
            break;
            
        case BL_CMD_GO:
            UART_Log(USART2, "Jump to Addr...\r\n");
            // Protocol: [ADDR 4B]
            Bootloader_JumpToUserApp(); 
            break;

        case 0x58: // BL_CMD_DEBUG_ECHO
            // Read 4 bytes and echo them back
            UART_WriteChar(USART2, 0x06); // ACK CMD
            UART_ReadBuffer(USART2, buffer, 4);
            UART_WriteBuffer(USART2, buffer, 4);
            break;
            
        case BL_CMD_READ_MEM:
             UART_WriteChar(USART2, 0x06); // ACK CMD
             
             // Protocol: [ADDR 4B] -> ACK -> [LEN 1B] -> ACK -> [DATA]
             UART_ReadBuffer(USART2, (uint8_t*)&addr, 4);
             UART_WriteChar(USART2, 0x06); // ACK ADDR
             
             len = (uint8_t)UART_ReadChar(USART2);
             UART_WriteChar(USART2, 0x06); // ACK LEN
             
             // Read from Memory and Send
             for (uint8_t i = 0; i < len; i++) {
                 uint8_t data = *((volatile uint8_t*)(addr + i));
                 UART_WriteChar(USART2, data);
             }
             break;
            
        default:
            UART_Log(USART2, "Unknown Command\r\n");
            break;
    }
}
