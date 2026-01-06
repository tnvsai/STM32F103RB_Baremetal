#ifndef __COMMON_H_
#define __COMMON_H_

#include "uart.h"

#define NULL    (void*)0
#define TRUE    (1)
#define FALSE   (0)


#define LOG_PREFIX "[LOG] "

#define UART_Log(usart, msg) do { \
    UART_WriteString(usart, LOG_PREFIX); \
    UART_WriteString(usart, msg); \
} while(0)

#define BUTTON_STATUS (GPIOC->IDR & (1 << 13))

// Memory regions
#define ADD_APP_END      0x08020000 // End of 128KB flash
#define ADD_APP_START    0x08008000      // Start of app region (after 32KB bootloader)
#define ADD_BL_START     0x08000000


// Bootloader command codes (sent from host PC over UART)
#define CMD_JUMP_BOOT     0x54
#define CMD_JUMP_APP      0x55
#define CMD_ERASE_APP     0x56
#define CMD_WRITE_MEM     0x57
#define CMD_READ_MEM      0x59
#define ACK               0x06
#define NACK              0x15
#define ERR               0xFF


// CRC Footer Structure
#define CRC_FOOTER_MAGIC    0xDEADBEEF
#define CRC_FOOTER_SIZE     16

// Signature Footer Structure (secure boot)
#define SIGNATURE_FOOTER_MAGIC 0xBEEFC0DE
#define SIGNATURE_FOOTER_SIZE  76 

#define SHARED_MEMORY_ADD   (uint32_t*)0x20004FFC
#define SHARED_MEMORY_VAL   *(SHARED_MEMORY_ADD)
#define SHARED_MAGIC_FLAG    0xC001D00D

#endif 