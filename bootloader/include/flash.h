#ifndef FLASH_H
#define FLASH_H

#include "stm32f103xb.h"
#include <stdint.h>

/* Flash page size for STM32F103RB */
#define FLASH_PAGE_SIZE      1024U

/* Application region (after 32KB bootloader) */
#define FLASH_START_ADDRESS  0x08008000U
#define FLASH_END_ADDRESS    0x0801FFFFU

/* Flash status */
typedef enum {
    FLASH_OK = 0,
    FLASH_ERROR_PROGRAM,
    FLASH_ERROR_WRP,
    FLASH_ERROR_BUSY
} Flash_Status_t;

/* --- Flash control --- */
void Flash_Unlock(void);
void Flash_Lock(void);

/* --- Flash operations --- */
Flash_Status_t Flash_EraseAppRegion(void);
Flash_Status_t Flash_ProgramHalfWord(uint32_t address, uint16_t data);
int Flash_Verify(uint32_t startAddr, const uint16_t *data, uint32_t length);

#endif
