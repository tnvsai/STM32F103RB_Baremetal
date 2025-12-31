#ifndef CRC_H
#define CRC_H

#include <stdint.h>

/**
 * @brief Initialize the hardware CRC peripheral
 * 
 * Enables RCC clock for CRC and resets the CRC calculation unit.
 * Must be called before using CRC functions.
 */
void CRC_Init(void);

/**
 * @brief Calculate CRC32 for word-aligned data
 * 
 * @param data Pointer to 32-bit word array
 * @param length_words Number of 32-bit words to process
 * @return uint32_t Calculated CRC-32 value
 * 
 * @note Uses STM32 hardware CRC (polynomial 0x04C11DB7)
 */
uint32_t CRC_Calculate(uint32_t *data, uint32_t length_words);

/**
 * @brief Calculate CRC32 for byte array
 * 
 * @param data Pointer to byte array
 * @param length_bytes Number of bytes to process
 * @return uint32_t Calculated CRC-32 value
 * 
 * @note Handles unaligned data by padding to word boundary
 */
uint32_t CRC_CalculateBytes(uint8_t *data, uint32_t length_bytes);

#endif // CRC_H
