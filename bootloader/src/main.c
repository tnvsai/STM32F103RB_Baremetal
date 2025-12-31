#include "stm32f103xb.h"
#include "uart.h"
#include "crc.h"
#include "flash.h"
#include <stddef.h> // For NULL

// Set to 0 for production (disables all logs for faster performance)
#define BL_DEBUG 1

#if BL_DEBUG
#define LOG_PREFIX "[LOG] "
    #define UART_Log(usart, msg) do { \
        UART_WriteString(usart, LOG_PREFIX); \
        UART_WriteString(usart, msg); \
    } while(0)
#else
#define UART_Log(usart, msg) ((void)0)
#endif

// Bootloader command codes (sent from host PC over UART)
#define BL_CMD_GO            0x55
#define BL_CMD_ERASE_APP     0x56
#define BL_CMD_WRITE_MEM     0x57
#define BL_CMD_READ_MEM      0x59

// CRC Footer Structure (placed immediately after application code)
#define CRC_FOOTER_MAGIC 0xDEADBEEF
#define FOOTER_SIZE 16            // 4 x uint32_t
#define APP_REGION_END 0x08020000 // End of 128KB flash
#define APP_START 0x08004000      // Start of app region

typedef struct {
  uint32_t size;    // Application code size in bytes
  uint32_t crc32;   // CRC32 checksum
  uint32_t version; // Firmware version (reserved for future)
  uint32_t magic;   // 0xDEADBEEF
} __attribute__((packed)) CRC_Footer_t;

void Bootloader_GPIO_Init(void);
void Bootloader_JumpToUserApp(void);
CRC_Footer_t *Bootloader_FindFooter(void);
void Bootloader_ProcessCommand(uint8_t cmd);

int main(void) 
{
  // Bootloader entry point

  // Initialize GPIO (button + LED)
  Bootloader_GPIO_Init();

  // Initialize CRC peripheral
  CRC_Init();

  // Configure UART for communication with host PC
  UART_Config_t uart2_cfg = {.baudRate = 115200,
                             .wordLength = UART_WORDLENGTH_8B,
                             .stopBits = UART_STOPBITS_1,
                             .parity = UART_PARITY_NONE,
                             .enableTx = 1,
                             .enableRx = 1};
  UART_Init(USART2, &uart2_cfg);

  if (GPIOC->IDR & (1 << 13)) {

    UART_Log(USART2, "Jumping to App...\r\n");
    Bootloader_JumpToUserApp();
  }

  // Bootloader mode active - print startup message
  UART_Log(USART2, "Bootloader Active v1.0\r\n");

  // Main bootloader loop: wait for commands from host
  while (1) {
    uint8_t cmd = (uint8_t)UART_ReadChar(USART2);
    Bootloader_ProcessCommand(cmd);
  }
}

void Bootloader_GPIO_Init(void) {
  // PC13: User button (input with pull-up)
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
  GPIOC->CRH &= ~(0xF << 20);
  GPIOC->CRH |= (0x8 << 20);
  GPIOC->ODR |= (1 << 13);

  // PA5: Status LED
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
  GPIOA->CRL &= ~(0xF << 20);
  GPIOA->CRL |= (0x2 << 20);
  GPIOA->ODR |= (1 << 5);
}

/**
 * Scan flash memory to locate the CRC footer
 * 
 * The footer is placed immediately after the application code (not at a fixed
 * address). We scan backward from the end of the app region looking for the
 * magic number 0xDEADBEEF.
 * 
 * Process:
 * 1. Start at end of app region (0x0801FFF0)
 * 2. Check if magic number exists at current address + 12 bytes
 * 3. If found, return pointer to footer structure
 * 4. If not found, move back 4 bytes and repeat
 * 5. Stop when we reach app start (0x08004000)
 * 
 * Worst case: ~28,000 checks (112KB / 4 bytes) = ~1-2ms at 72MHz
 * 
 * @return Pointer to footer if found, NULL if not found
 */
CRC_Footer_t *Bootloader_FindFooter(void) {
  // Start from end of app region and scan backward
  for (uint32_t addr = APP_REGION_END - FOOTER_SIZE; addr >= APP_START;
       addr -= 4) {
    
    // Check magic number at offset +12 (4th field in footer structure)
    // Footer layout: [size:4][crc32:4][version:4][magic:4]
    //                                              ↑ we check this
    uint32_t magic = *((volatile uint32_t *)(addr + 12));
    
    if (magic == CRC_FOOTER_MAGIC) {
      // Found valid footer!
      return (CRC_Footer_t *)addr;
    }
  }
  
  // Footer not found (firmware without CRC footer)
  return NULL;
}

/**
 * Verify application firmware and jump to it if valid
 * 
 * Process:
 * 1. Validate application has valid stack pointer (in RAM)
 * 2. Search for CRC footer in flash memory
 * 3. If footer found:
 *    a. Calculate CRC-32 of application code
 *    b. Compare with stored CRC in footer
 *    c. Jump to app if match, stay in bootloader if mismatch
 * 4. If no footer found (old firmware), skip CRC and jump anyway
 * 
 * Note: CRC verification happens on EVERY reset when button not pressed.
 * This protects against flash corruption, cosmic rays, or tampering.
 */
void Bootloader_JumpToUserApp(void) {
  uint32_t app_addr = FLASH_START_ADDRESS;

  // Step 1: Read application's initial stack pointer (first word of vector table)
  uint32_t msp_value = *((volatile uint32_t *)app_addr);

  // Step 2: Validate MSP is in valid RAM range (0x20000000-0x20005000)
  if ((msp_value & 0x2FF00000) == 0x20000000) {
    
    // Step 3: Search for CRC footer by scanning flash memory
    CRC_Footer_t *footer = Bootloader_FindFooter();

    if (footer != NULL) {
      // Footer found! Perform CRC verification
      UART_Log(USART2, "CRC footer found. Verifying...\r\n");

      // Step 4: Calculate CRC-32 of application code
      // Uses size from footer (ignores 0xFF padding after code)
      uint8_t *app_code = (uint8_t *)app_addr;
      uint32_t calculated_crc = CRC_CalculateBytes(app_code, footer->size);

      // Step 5: Compare calculated CRC with stored CRC
      if (calculated_crc == footer->crc32) {
        // CRC match - firmware is valid!
        UART_Log(USART2, "CRC OK!\r\n");
      } else {
        // CRC mismatch - firmware is corrupted!
        UART_Log(USART2, "CRC FAIL! Firmware corrupted.\r\n");
        
        // Display expected vs calculated CRC for debugging
        UART_WriteString(USART2, "Expected: 0x");
        UART_WriteHex8(USART2, (footer->crc32 >> 24) & 0xFF);
        UART_WriteHex8(USART2, (footer->crc32 >> 16) & 0xFF);
        UART_WriteHex8(USART2, (footer->crc32 >> 8) & 0xFF);
        UART_WriteHex8(USART2, footer->crc32 & 0xFF);
        UART_WriteString(USART2, "\r\nCalculated: 0x");
        UART_WriteHex8(USART2, (calculated_crc >> 24) & 0xFF);
        UART_WriteHex8(USART2, (calculated_crc >> 16) & 0xFF);
        UART_WriteHex8(USART2, (calculated_crc >> 8) & 0xFF);
        UART_WriteHex8(USART2, calculated_crc & 0xFF);
        UART_WriteString(USART2, "\r\n");
        
        // DO NOT BOOT corrupted firmware - stay in bootloader
        return;
      }
    } else {
      // No footer found - probably old firmware without CRC
      // Skip verification and boot anyway (backward compatibility)
      UART_Log(USART2, "No CRC footer (old firmware). Skipping check.\r\n");
    }

    // Set stack pointer to application's value
    __set_MSP(msp_value);

    // Read application's reset handler address (second word of vector table)
    uint32_t reset_handler_addr = *((volatile uint32_t *)(app_addr + 4));
    void (*app_reset_handler)(void) = (void *)reset_handler_addr;

    // Jump to application! (never returns)
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

  switch (cmd) {
  case BL_CMD_ERASE_APP:
    UART_Log(USART2, "Erasing...\r\n");
    if (Flash_EraseAppRegion() == FLASH_OK) {
      UART_Log(USART2, "Result: OK\r\n");
      UART_WriteChar(USART2, 0x06);
    } else {
      UART_Log(USART2, "Result: FAIL\r\n");
      UART_WriteChar(USART2, 0x15);
    }
    break;

  case BL_CMD_WRITE_MEM:
            // Protocol: [CMD] -> ACK -> [ADDR 4B] -> ACK -> [LEN 1B] -> ACK -> [DATA] -> ACK/NACK
    UART_WriteChar(USART2, 0x06);
    UART_ReadBuffer(USART2, (uint8_t *)&addr, 4);
    UART_WriteChar(USART2, 0x06);

    len = (uint8_t)UART_ReadChar(USART2);
    UART_WriteChar(USART2, 0x06);

    if (len > 64)
      len = 64;

    UART_ReadBuffer(USART2, buffer, len);

    uint8_t status = 0x06;

    // Prepare flash for writing
    Flash_Unlock();
    FLASH->SR |= (FLASH_SR_PGERR | FLASH_SR_WRPRTERR); // Clear error flags

    // Write in 16-bit chunks (Flash requires halfword writes)
    for (int i = 0; i < len; i += 2) {
      uint16_t data = buffer[i] | (buffer[i + 1] << 8);

      if ((i % 2) == 0)
        GPIOA->ODR ^= (1 << 5);

      // Read current flash value
      uint16_t current_val = *((volatile uint16_t *)(addr + i));

      if (current_val == data) {
        continue; // Skip write if already correct (faster + prevents errors)
      }

      // Write 16-bit data to flash
      Flash_Status_t f_status = Flash_ProgramHalfWord(addr + i, data);
      if (f_status != FLASH_OK) {
        status = (uint8_t)f_status;
        break;
      }

      // Small delay for flash controller stability
      for (volatile int d = 0; d < 1000; d++)
        ;
    }

    // Lock flash to prevent accidental writes
    Flash_Lock();

    UART_WriteChar(USART2, status);
    break;

  case BL_CMD_GO:
    // Jump to application command
    UART_Log(USART2, "Jump to Addr...\r\n");

    Bootloader_JumpToUserApp();
    break;

  case 0x58:
    UART_WriteChar(USART2, 0x06);
    UART_ReadBuffer(USART2, buffer, 4);
    UART_WriteBuffer(USART2, buffer, 4);
    break;

  case BL_CMD_READ_MEM:
    // Protocol: [CMD] -> ACK -> [ADDR 4B] -> ACK -> [LEN 1B] -> ACK -> [DATA]
    GPIOA->ODR ^= (1 << 5);
    UART_WriteChar(USART2, 0x06);
    UART_ReadBuffer(USART2, (uint8_t *)&addr, 4);
    UART_WriteChar(USART2, 0x06);
    len = (uint8_t)UART_ReadChar(USART2);
    UART_WriteChar(USART2, 0x06);
    for (uint8_t i = 0; i < len; i++) {
      uint8_t data = *((volatile uint8_t *)(addr + i));
      UART_WriteChar(USART2, data);
    }
    GPIOA->ODR ^= (1 << 5);
    break;

  default:
    UART_Log(USART2, "Unknown Command\r\n");
    break;
  }
}
