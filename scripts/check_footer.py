#!/usr/bin/env python3
"""
STM32 CRC Footer Verification Script
=====================================

This script verifies the CRC footer in an STM32 application binary.

WHAT IT DOES:
-------------
1. Reads the application binary
2. Locates the CRC footer at the end
3. Validates the footer structure
4. Displays footer contents and status

USAGE:
------
    python verify_footer.py <binary_file.bin>

OUTPUT:
-------
Shows footer information including:
- Binary size
- Footer location
- Code size from footer
- CRC-32 checksum
- Magic number validation

This is useful for debugging and verifying that the CRC injection
worked correctly before flashing to the board.
"""

import sys
import struct
from pathlib import Path

# Footer constants (must match inject_crc.py)
FOOTER_SIZE = 16
FOOTER_MAGIC = 0xDEADBEEF


def verify_footer(bin_path):
    """
    Verify CRC footer in binary file and display information.
    
    Args:
        bin_path (str): Path to binary file to verify
        
    Returns:
        bool: True if footer is valid, False otherwise
    """
    print(f"Verifying CRC Footer")
    print("=" * 60)
    print(f"File: {bin_path}")
    print()
    
    # Read binary file
    try:
        with open(bin_path, 'rb') as f:
            data = f.read()
    except FileNotFoundError:
        print(f"❌ ERROR: File not found: {bin_path}")
        return False
    except IOError as e:
        print(f"❌ ERROR: Cannot read file: {e}")
        return False
    
    total_size = len(data)
    print(f"Binary size: {total_size} bytes (0x{total_size:X})")
    
    # Check if binary is large enough for footer
    if total_size < FOOTER_SIZE:
        print(f"❌ ERROR: Binary too small for footer ({total_size} < {FOOTER_SIZE} bytes)")
        return False
    
    # Extract footer (last 16 bytes)
    footer_offset = total_size - FOOTER_SIZE
    footer_bytes = data[footer_offset:]
    
    # Parse footer structure
    try:
        size, crc32, version, magic = struct.unpack('<IIII', footer_bytes)
    except struct.error as e:
        print(f"❌ ERROR: Cannot parse footer: {e}")
        return False
    
    print()
    print("Footer Information")
    print("-" * 60)
    print(f"Location:        Offset 0x{footer_offset:X} (byte {footer_offset})")
    print(f"Flash address:   0x{0x08004000 + footer_offset:08X}")
    print()
    print(f"Code size:       {size} bytes (0x{size:X})")
    print(f"CRC-32:          0x{crc32:08X}")
    print(f"Version:         {version}")
    print(f"Magic:           0x{magic:08X}")
    
    # Validate magic number
    print()
    if magic == FOOTER_MAGIC:
        print("✅ Footer magic valid (0xDEADBEEF)")
    else:
        print(f"❌ Invalid magic! Expected 0x{FOOTER_MAGIC:08X}, got 0x{magic:08X}")
        return False
    
    # Validate size field
    if size > total_size - FOOTER_SIZE:
        print(f"❌ Invalid size! Code size ({size}) exceeds binary size")
        return False
    
    if size == 0:
        print("❌ Invalid size! Code size is zero")
        return False
    
    # Calculate actual code size
    code_bytes = total_size - FOOTER_SIZE
    if size == code_bytes:
        print(f"✅ Code size matches ({size} bytes)")
    else:
        print(f"⚠️  Code size mismatch:")
        print(f"   Footer says: {size} bytes")
        print(f"   Actual code: {code_bytes} bytes")
    
    # Show memory layout
    print()
    print("Memory Layout")
    print("-" * 60)
    print(f"0x00000000: [Code: {size} bytes]")
    print(f"0x{size:08X}: [Footer: {FOOTER_SIZE} bytes]")
    if total_size > size + FOOTER_SIZE:
        padding = total_size - size - FOOTER_SIZE
        print(f"0x{size + FOOTER_SIZE:08X}: [Padding: {padding} bytes] (should be 0)")
    print(f"0x{total_size:08X}: [End of binary]")
    
    print()
    print("✅ Footer verification successful!")
    return True


if __name__ == "__main__":
    # Check command-line arguments
    if len(sys.argv) != 2:
        print("STM32 CRC Footer Verification Script")
        print("=" * 40)
        print()
        print("Usage: python verify_footer.py <binary_file.bin>")
        print()
        print("This script verifies the CRC footer in your firmware binary.")
        print("Use it to check that the footer was correctly added before flashing.")
        print()
        sys.exit(1)
    
    bin_file = Path(sys.argv[1])
    
    # Verify file exists
    if not bin_file.exists():
        print(f"ERROR: File not found: {bin_file}")
        sys.exit(1)
    
    # Verify footer
    success = verify_footer(bin_file)
    
    # Exit with appropriate code
    sys.exit(0 if success else 1)
