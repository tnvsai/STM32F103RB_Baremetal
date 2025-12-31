#include "stm32f103xb.h"
#include "uart.h"
#include "crc.h"
#include "flash.h"
#include "crypto_wrapper.h"
#include "public_key.h"

#define NULL    (void*)0
#define TRUE    (1)
#define FALSE   (0)

#define BL_LOG_ENABLE     TRUE

#if BL_LOG_ENABLE
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

// Memory regions
#define APP_REGION_END      0x08020000 // End of 128KB flash
#define APP_START           0x08008000      // Start of app region (after 32KB bootloader)

// CRC Footer Structure (legacy support)
#define CRC_FOOTER_MAGIC    0xDEADBEEF
#define CRC_FOOTER_SIZE     16

// Signature Footer Structure (secure boot)
#define SIGNATURE_FOOTER_MAGIC 0xBEEFC0DE
#define SIGNATURE_FOOTER_SIZE  76 


typedef struct {
  uint32_t size;    // Application code size in bytes
  uint32_t crc32;   // CRC32 checksum
  uint32_t version; // Firmware version (reserved for future)
  uint32_t magic;   // 0xDEADBEEF
} __attribute__((packed)) CRC_Footer_t;

typedef struct {
  uint32_t firmware_size;    // Size of firmware in bytes (excluding footer)
  uint32_t version;          // Firmware version number
  uint8_t  signature[64];    // ECDSA signature (R=32, S=32)
  uint32_t magic;            // 0xBEEFC0DE
} __attribute__((packed)) SignatureFooter_t;  // Total: 76 bytes


void Bootloader_GPIO_Init(void);
void Bootloader_JumpToUserApp(void);
CRC_Footer_t *Bootloader_FindCrcFooter(void);
SignatureFooter_t *Bootloader_FindSignatureFooter(void);
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
 * 5. Stop when we reach app start (0x08008000)
 * 
 * Worst case: ~24,000 checks (96KB / 4 bytes) = ~1-2ms at 72MHz
 * 
 * @return Pointer to footer if found, NULL if not found
 */
CRC_Footer_t *Bootloader_FindCrcFooter(void) {
  // Start from end of app region and scan backward
  for (uint32_t addr = APP_REGION_END - CRC_FOOTER_SIZE; addr >= APP_START;
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
 * Scan flash memory to locate the signature footer
 * 
 * Similar to CRC footer scan, but looking for 0xBEEFC0DE magic number.
 * The signature footer is larger (108 bytes) and placed after the firmware.
 * 
 * @return Pointer to signature footer if found, NULL if not found
 */
SignatureFooter_t *Bootloader_FindSignatureFooter(void) {
  // Start from end of app region and scan backward
  for (uint32_t addr = APP_REGION_END - SIGNATURE_FOOTER_SIZE; addr >= APP_START; addr -= 4) 
  { 
    // Check magic number at offset +72 (last field: firmware_size(4) + version(4) + sig(64) + magic(4))
    uint32_t magic = *((volatile uint32_t *)(addr + 72));
    
    if (magic == SIGNATURE_FOOTER_MAGIC) {
      // Found valid signature footer!
      return (SignatureFooter_t *)addr;
    }
  }
  
  // Signature footer not found
  return NULL;
}

/**
 * Verify application firmware and jump to it if valid
 * 
 * SECURE BOOT PROCESS:
 * 1. Validate application has valid stack pointer (in RAM)
 * 2. Search for signature footer in flash memory
 * 3. If signature found:
 *    a. Calculate SHA-256 hash of firmware
 *    b. Verify ECDSA signature using embedded public key
 *    c. Jump to app if signature valid, refuse boot if invalid
 * 4. Fallback to CRC verification for backward compatibility
 * 
 * Note: Signature verification provides cryptographic authentication.
 * Only firmware signed with the corresponding private key will boot.
 */
void Bootloader_JumpToUserApp(void) {
  uint32_t app_addr = APP_START;

  // Step 1: Read application's initial stack pointer (first word of vector table)
  uint32_t msp_value = *((volatile uint32_t *)app_addr);

  // Step 2: Validate MSP is in valid RAM range (0x20000000-0x20005000)
  if ((msp_value & 0x2FF00000) != 0x20000000) {
    UART_Log(USART2, "Invalid MSP - BOOT DENIED\r\n");
    return;
  }

  // Step 3: Search for secure boot signature footer
  SignatureFooter_t *sig_footer = Bootloader_FindSignatureFooter();

  if (sig_footer != NULL) {
    // ==============================================
    // SECURE BOOT: Signature Verification
    // ==============================================
    UART_Log(USART2, "Signature found. Verifying...\r\n");

    // Step 3a: Calculate SHA-256 hash of firmware
    uint8_t calculated_hash[32];
    Crypto_SHA256((uint8_t *)app_addr, sig_footer->firmware_size, calculated_hash);

    // Step 3b: Verify ECDSA signature
    extern const uint8_t PUBLIC_KEY[64];
    
    int signature_valid = Crypto_VerifySignature(
        calculated_hash, 32,
        sig_footer->signature,
        PUBLIC_KEY
    );

    if (signature_valid) {
      // Signature is VALID - firmware authenticated!
      UART_Log(USART2, "Signature VALID - Booting...\r\n");

      // Set stack pointer and jump to application
      __set_MSP(msp_value);
      uint32_t reset_handler_addr = *((volatile uint32_t *)(app_addr + 4));
      void (*app_reset_handler)(void) = (void *)reset_handler_addr;
      app_reset_handler(); // Jump! (never returns)
      
    } else {
      // Signature is INVALID - firmware tampered or not signed properly
      UART_Log(USART2, "Signature INVALID - BOOT DENIED\r\n");
      return; // Refuse to boot
    }

  } else {
    // ==============================================
    // FALLBACK: CRC Verification (Legacy Support)
    // ==============================================
    UART_Log(USART2, "No signature. Trying CRC...\r\n");
    
    CRC_Footer_t *crc_footer = Bootloader_FindFooter();

    if (crc_footer != NULL) {
      // Legacy CRC verification
      UART_Log(USART2, "CRC footer found. Verifying...\r\n");

      uint8_t *app_code = (uint8_t *)app_addr;
      uint32_t calculated_crc = CRC_CalculateBytes(app_code, crc_footer->size);

      if (calculated_crc == crc_footer->crc32) {
        UART_Log(USART2, "CRC OK - Booting (legacy)...\r\n");
      } else {
        UART_Log(USART2, "CRC FAIL - BOOT DENIED\r\n");
        return;
      }
    } else {
      // No signature AND no CRC footer - REFUSE TO BOOT (fail-closed)
      UART_Log(USART2, "No footer - BOOT DENIED\r\n");
      return;
    }

    // CRC passed - boot the app
    __set_MSP(msp_value);
    uint32_t reset_handler_addr = *((volatile uint32_t *)(app_addr + 4));
    void (*app_reset_handler)(void) = (void *)reset_handler_addr;
    app_reset_handler(); // Jump! (never returns)
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