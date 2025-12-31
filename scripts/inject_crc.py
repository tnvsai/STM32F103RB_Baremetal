#!/usr/bin/env python3
"""
STM32 CRC Footer Injection Script
==================================

This script appends a CRC-32 footer to an STM32 application binary for bootloader verification.

WHAT IT DOES:
-------------
1. Reads the compiled application binary
2. Calculates CRC-32 checksum using STM32-compatible polynomial
3. Appends a 16-byte footer containing:
   - Firmware size (4 bytes)
   - CRC-32 checksum (4 bytes)
   - Version number (4 bytes) - reserved for future use
   - Magic number (4 bytes) - 0xDEADBEEF for validation

HOW IT WORKS:
-------------
The bootloader scans flash memory for the magic number 0xDEADBEEF to locate
the footer, then uses the size and CRC fields to verify firmware integrity
before booting.

USAGE:
------
    python add_crc_footer.py <binary_file.bin>

OUTPUT:
-------
Modifies the input binary in-place by appending the CRC footer.
Final binary size = original_size + 16 bytes

EXAMPLE:
--------
    Input:  app.bin (5860 bytes of code)
    Output: app.bin (5876 bytes = 5860 code + 16 footer)

IMPORTANT:
----------
- Uses STM32 hardware CRC polynomial: 0x04C11DB7
- This MUST match the bootloader's CRC calculation
- Binary must fit within app region (default: 112KB)
"""

import sys
import struct
from pathlib import Path

# =============================================================================
# CONFIGURATION
# =============================================================================

# CRC polynomial used by STM32 hardware CRC peripheral
# This MUST match the bootloader implementation!
CRC_POLY = 0x04C11DB7

# Initial CRC value (all bits set)
CRC_INIT = 0xFFFFFFFF

# Maximum application region size (128KB flash - 16KB bootloader)
APP_REGION_SIZE = 0x1C000  # 112KB

# Footer structure size (4 x uint32_t)
FOOTER_SIZE = 16

# Magic number to identify valid footer
# Bootloader searches for this pattern to locate the footer
FOOTER_MAGIC = 0xDEADBEEF


# =============================================================================
# CRC CALCULATION (STM32-Compatible)
# =============================================================================

def crc32_stm32(data):
    """
    Calculate CRC-32 checksum using STM32 hardware polynomial.
    
    This implementation matches the behavior of the STM32F1 CRC peripheral
    to ensure consistency between build-time calculation and runtime verification.
    
    Algorithm:
    ----------
    1. Process data in 32-bit words (little-endian)
    2. XOR each word with current CRC
    3. For each bit, if MSB is 1, shift and XOR with polynomial
    4. Return final CRC value
    
    Args:
        data (bytes): Firmware binary data to checksum
        
    Returns:
        int: 32-bit CRC checksum
        
    Note:
        Incomplete words are zero-padded to maintain 4-byte alignment.
    """
    crc = CRC_INIT
    
    # Process data in 32-bit chunks (STM32 CRC works on words)
    for i in range(0, len(data), 4):
        # Extract 32-bit word (little-endian byte order)
        if i + 4 <= len(data):
            word = struct.unpack('<I', data[i:i+4])[0]
        else:
            # Pad incomplete word with zeros
            remaining = data[i:]
            word = struct.unpack('<I', remaining + b'\x00' * (4 - len(remaining)))[0]
        
        # XOR word into CRC
        crc ^= word
        
        # Process each bit (CRC polynomial division)
        for _ in range(32):
            if crc & 0x80000000:  # If MSB is 1
                crc = ((crc << 1) ^ CRC_POLY) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    
    return crc


# =============================================================================
# FOOTER INJECTION
# =============================================================================

def inject_footer(bin_path):
    """
    Append CRC footer to application binary.
    
    This function:
    1. Reads the existing binary file
    2. Calculates CRC-32 of the entire code
    3. Appends a footer with size, CRC, and magic number
    4. Writes the modified binary back to disk
    
    Footer Structure (16 bytes, little-endian):
    -------------------------------------------
    Offset | Size | Field       | Description
    -------|------|-------------|----------------------------------
    +0     | 4    | size        | Firmware code size in bytes
    +4     | 4    | crc32       | CRC-32 checksum of code
    +8     | 4    | version     | Firmware version (reserved)
    +12    | 4    | magic       | 0xDEADBEEF (footer signature)
    
    Args:
        bin_path (str): Path to binary file to process
        
    Returns:
        bool: True if successful, False if error occurred
        
    Note:
        The input binary is modified IN-PLACE. Make a backup if needed!
    """
    print(f"[FOOTER] Processing: {bin_path}")
    
    # Read existing binary file
    try:
        with open(bin_path, 'rb') as f:
            data = bytearray(f.read())
    except FileNotFoundError:
        print(f"[FOOTER] ERROR: File not found: {bin_path}")
        return False
    except IOError as e:
        print(f"[FOOTER] ERROR: Cannot read file: {e}")
        return False
    
    code_size = len(data)
    print(f"[FOOTER] Code size: {code_size} bytes (0x{code_size:X})")
    
    # Verify binary fits in application region (with footer)
    if code_size > APP_REGION_SIZE - FOOTER_SIZE:
        print(f"[FOOTER] ERROR: Binary too large!")
        print(f"         Size: {code_size} bytes")
        print(f"         Maximum: {APP_REGION_SIZE - FOOTER_SIZE} bytes")
        return False
    
    # Calculate CRC-32 of firmware code
    print(f"[FOOTER] Calculating CRC-32...")
    crc = crc32_stm32(data)
    print(f"[FOOTER] CRC-32: 0x{crc:08X}")
    
    # Build footer structure (16 bytes, little-endian)
    #   [size:4] [crc32:4] [version:4] [magic:4]
    footer = struct.pack(
        '<IIII',        # Format: 4 unsigned 32-bit ints, little-endian
        code_size,      # Firmware size (bootloader uses this for CRC calculation)
        crc,            # CRC-32 checksum
        0,              # Version (reserved for future use)
        FOOTER_MAGIC    # Magic number (bootloader searches for this)
    )
    
    # Append footer to binary
    data.extend(footer)
    
    # Write modified binary back to disk
    try:
        with open(bin_path, 'wb') as f:
            f.write(data)
    except IOError as e:
        print(f"[FOOTER] ERROR: Cannot write file: {e}")
        return False
    
    # Report results
    final_size = len(data)
    footer_offset = code_size
    
    print(f"[FOOTER] Footer appended successfully!")
    print(f"         Final size: {final_size} bytes ({code_size} code + 16 footer)")
    print(f"         Footer location: offset 0x{footer_offset:X}")
    print(f"         Flash address: 0x{0x08004000 + footer_offset:08X}")
    
    return True


# =============================================================================
# MAIN ENTRY POINT
# =============================================================================

if __name__ == "__main__":
    # Check command-line arguments
    if len(sys.argv) != 2:
        print("STM32 CRC Footer Injection Script")
        print("=" * 40)
        print()
        print("Usage: python add_crc_footer.py <binary_file.bin>")
        print()
        print("This script appends a 16-byte CRC footer to your firmware")
        print("for bootloader verification. The footer contains:")
        print("  - Firmware size")
        print("  - CRC-32 checksum")
        print("  - Version number (reserved)")
        print("  - Magic number (0xDEADBEEF)")
        print()
        sys.exit(1)
    
    bin_file = Path(sys.argv[1])
    
    # Verify file exists
    if not bin_file.exists():
        print(f"ERROR: File not found: {bin_file}")
        sys.exit(1)
    
    # Inject CRC footer
    success = inject_footer(bin_file)
    
    # Exit with appropriate code
    sys.exit(0 if success else 1)
