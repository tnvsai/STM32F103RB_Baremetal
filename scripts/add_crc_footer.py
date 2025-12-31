#!/usr/bin/env python3
"""
CRC Footer Generator
Adds CRC-32 footer to firmware binary for legacy bootloader support
"""

import struct
import sys
from pathlib import Path

# CRC Footer Structure (16 bytes)
# typedef struct {
#   uint32_t size;    // Application code size in bytes
#   uint32_t crc32;   // CRC32 checksum
#   uint32_t version; // Firmware version (reserved)
#   uint32_t magic;   // 0xDEADBEEF
# } __attribute__((packed)) CRC_Footer_t;

CRC_FOOTER_MAGIC = 0xDEADBEEF
FIRMWARE_VERSION = 1  # Can be updated manually or via build system

def calculate_crc32_stm32(data):
    """
    Calculate CRC-32 using STM32 hardware CRC algorithm
    
    STM32 CRC peripheral uses:
    - Polynomial: 0x04C11DB7 (Ethernet CRC-32)
    - Initial value: 0xFFFFFFFF
    - Input: 32-bit words (little-endian)
    - Output: No final XOR
    
    This matches the STM32F1 hardware CRC peripheral behavior.
    """
    crc = 0xFFFFFFFF
    poly = 0x04C11DB7
    
    # Process data in 32-bit words
    num_words = len(data) // 4
    for i in range(num_words):
        # Read 32-bit word (little-endian)
        word = int.from_bytes(data[i*4:(i+1)*4], byteorder='little')
        crc ^= word
        
        # Process 32 bits
        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ poly) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    
    # Handle remaining bytes (if not multiple of 4)
    remaining = len(data) % 4
    if remaining > 0:
        last_word = 0
        for i in range(remaining):
            last_word |= data[num_words * 4 + i] << (i * 8)
        
        crc ^= last_word
        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ poly) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    
    return crc

def add_crc_footer(binary_path):
    """Add CRC footer to binary"""
    bin_file = Path(binary_path)
    
    if not bin_file.exists():
        print(f"Error: File not found: {binary_path}")
        return False
    
    # Read firmware binary
    with open(bin_file, 'rb') as f:
        firmware_data = f.read()
    
    firmware_size = len(firmware_data)
    
    # Calculate CRC-32 using STM32 hardware algorithm
    crc32_value = calculate_crc32_stm32(firmware_data)
    
    # Create footer (little-endian)
    footer = struct.pack('<IIII',
        firmware_size,      # size
        crc32_value,        # crc32
        FIRMWARE_VERSION,   # version
        CRC_FOOTER_MAGIC    # magic
    )
    
    # Write firmware + footer
    with open(bin_file, 'wb') as f:
        f.write(firmware_data)
        f.write(footer)
    
    # Print summary
    print(f"[*] CRC Footer Added")
    print(f"    Firmware Size: {firmware_size} bytes")
    print(f"    CRC-32: 0x{crc32_value:08X}")
    print(f"    Version: {FIRMWARE_VERSION}")
    print(f"    Footer Size: 16 bytes")
    print(f"    Total: {firmware_size + 16} bytes")
    print(f"[+] Firmware ready for legacy CRC bootloader!")
    
    return True

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python add_crc_footer.py <firmware.bin>")
        sys.exit(1)
    
    binary_path = sys.argv[1]
    
    if not add_crc_footer(binary_path):
        sys.exit(1)
