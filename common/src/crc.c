/*
 * CRC Driver for STM32F103
 * 
 * Uses hardware CRC peripheral for fast CRC-32 calculation.
 * Polynomial: 0x04C11DB7 (CRC-32/MPEG-2)
 * Initial value: 0xFFFFFFFF
 */

#include "crc.h"
#include "stm32f103xb.h"

/**
 * Initialize the hardware CRC peripheral
 * 
 * Step 1: Enable CRC peripheral clock via RCC
 * Step 2: Reset CRC calculation unit to initial state (0xFFFFFFFF)
 */
void CRC_Init(void) {
    // Enable CRC clock on AHB bus
    RCC->AHBENR |= RCC_AHBENR_CRCEN;
    
    // Reset CRC calculation unit (sets DR to 0xFFFFFFFF)
    CRC->CR = CRC_CR_RESET;
}

/**
 * Calculate CRC-32 for word-aligned data
 * 
 * Process:
 * 1. Reset CRC unit to 0xFFFFFFFF
 * 2. Feed each 32-bit word to hardware
 * 3. Hardware automatically updates CRC after each word
 * 4. Read final CRC value from data register
 * 
 * @param data Pointer to 32-bit word array
 * @param length_words Number of words to process
 * @return Calculated CRC-32 checksum
 */
uint32_t CRC_Calculate(uint32_t *data, uint32_t length_words) {
    // Step 1: Reset CRC to initial value
    CRC->CR = CRC_CR_RESET;
    
    // Step 2: Feed each word to hardware CRC unit
    for (uint32_t i = 0; i < length_words; i++) {
        CRC->DR = data[i];  // Writing to DR triggers CRC calculation
    }
    
    // Step 3: Read final CRC result
    return CRC->DR;
}

/**
 * Calculate CRC-32 for byte array (handles unaligned data)
 * 
 * Process:
 * 1. Reset CRC unit
 * 2. Process complete 32-bit words
 * 3. Pack remaining bytes (if any) into final word with zero-padding
 * 4. Read final CRC value
 * 
 * Example: 10 bytes
 *   - Process bytes 0-7 as 2 complete words
 *   - Pack bytes 8-9 as: 0x00000908 (little-endian, zero-padded)
 * 
 * @param data Pointer to byte array
 * @param length_bytes Number of bytes to process
 * @return Calculated CRC-32 checksum
 */
uint32_t CRC_CalculateBytes(uint8_t *data, uint32_t length_bytes) {
    // Step 1: Reset CRC to initial value
    CRC->CR = CRC_CR_RESET;
    
    // Step 2: Process complete 32-bit words
    uint32_t num_words = length_bytes / 4;
    uint32_t *word_ptr = (uint32_t *)data;
    
    for (uint32_t i = 0; i < num_words; i++) {
        CRC->DR = word_ptr[i];
    }
    
    // Step 3: Handle remaining bytes (if length not multiple of 4)
    uint32_t remaining = length_bytes % 4;
    if (remaining > 0) {
        uint32_t last_word = 0;  // Zero-padded word
        uint8_t *byte_ptr = data + (num_words * 4);
        
        // Pack remaining bytes into 32-bit word (little-endian)
        // Example: [0xAA, 0xBB] becomes 0x0000BBAA
        for (uint32_t i = 0; i < remaining; i++) {
            last_word |= ((uint32_t)byte_ptr[i]) << (i * 8);
        }
        
        CRC->DR = last_word;
    }
    
    // Step 4: Read final CRC result
    return CRC->DR;
}
