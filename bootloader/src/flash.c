#include "flash.h"

/* --- Flash control --- */

void Flash_Unlock(void) {
    if (!(FLASH->CR & FLASH_CR_LOCK)) return; // already unlocked
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
}

void Flash_Lock(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}

Flash_Status_t Flash_WaitForLastOperation(void) {
    while (FLASH->SR & FLASH_SR_BSY); // wait busy

    Flash_Status_t status = FLASH_OK;

    if (FLASH->SR & FLASH_SR_PGERR) {
        status = FLASH_ERROR_PROGRAM;
        FLASH->SR |= FLASH_SR_PGERR; // clear flag
    }

    if (FLASH->SR & FLASH_SR_WRPRTERR) {
        status = FLASH_ERROR_WRP;
        FLASH->SR |= FLASH_SR_WRPRTERR; // clear flag
    }

    return status;
}

/* --- Flash operations --- */

// Erase single page
Flash_Status_t Flash_ErasePage(uint32_t pageAddress) {
    Flash_Unlock();

    FLASH->CR &= ~FLASH_CR_PER;       // clear previous PER
    FLASH->CR |= FLASH_CR_PER;        // enable page erase
    FLASH->AR = pageAddress;          // page address
    FLASH->CR |= FLASH_CR_STRT;       // start erase

    Flash_Status_t status = Flash_WaitForLastOperation();
    FLASH->CR &= ~FLASH_CR_PER;       // disable page erase

    Flash_Lock();
    return status;
}

// Erase all application region
Flash_Status_t Flash_EraseAppRegion(void) {
    Flash_Status_t status;

    for (uint32_t addr = FLASH_START_ADDRESS; addr <= FLASH_END_ADDRESS; addr += FLASH_PAGE_SIZE) {
        status = Flash_ErasePage(addr);
        if (status != FLASH_OK) return status;
    }

    return FLASH_OK;
}

// Program a 16-bit half-word
Flash_Status_t Flash_ProgramHalfWord(uint32_t address, uint16_t data) {
    if (address & 0x1) return FLASH_ERROR_PROGRAM; // must be half-word aligned

    Flash_Unlock();
    FLASH->CR |= FLASH_CR_PG;                  // enable programming
    *((volatile uint16_t *)address) = data;
    Flash_Status_t status = Flash_WaitForLastOperation();
    FLASH->CR &= ~FLASH_CR_PG;                 // disable programming
    Flash_Lock();

    return status;
}

// Verify flash content
int Flash_Verify(uint32_t startAddr, uint16_t *data, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        if (*((volatile uint16_t *)(startAddr + i*2)) != data[i])
            return 0;
    }
    return 1;
}
