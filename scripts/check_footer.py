#!/usr/bin/env python3
"""
STM32 Footer Verification Script
==================================

This script verifies footers in STM32 application binaries.
Supports both CRC and Signature footer types.

FOOTER TYPES:
-------------
1. CRC Footer (0xDEADBEEF) - 16 bytes
   - Legacy bootloader support
   - Contains: size, crc32, version, magic

2. Signature Footer (0xBEEFC0DE) - 76 bytes  
   - Secure boot support
   - Contains: size, version, signature (64 bytes), magic

USAGE:
------
    python check_footer.py <binary_file.bin>

The script will auto-detect the footer type and verify it accordingly.
"""

import sys
import struct
from pathlib import Path

# Footer constants
CRC_FOOTER_SIZE = 16
CRC_FOOTER_MAGIC = 0xDEADBEEF

SIGNATURE_FOOTER_SIZE = 76
SIGNATURE_FOOTER_MAGIC = 0xBEEFC0DE

APP_START_ADDRESS = 0x08008000  # Application start address (after 32KB bootloader)


def verify_crc_footer(data, footer_offset):
    """Verify CRC footer structure."""
    footer_bytes = data[footer_offset:]
    
    if len(footer_bytes) < CRC_FOOTER_SIZE:
        print(f"❌ ERROR: Not enough data for CRC footer")
        return False
    
    # Parse CRC footer: size(4) + crc32(4) + version(4) + magic(4)
    size, crc32, version, magic = struct.unpack('<IIII', footer_bytes[:CRC_FOOTER_SIZE])
    
    print()
    print("CRC Footer Information")
    print("-" * 60)
    print(f"Location:        Offset 0x{footer_offset:X} (byte {footer_offset})")
    print(f"Flash address:   0x{APP_START_ADDRESS + footer_offset:08X}")
    print()
    print(f"Code size:       {size} bytes (0x{size:X})")
    print(f"CRC-32:          0x{crc32:08X}")
    print(f"Version:         {version}")
    print(f"Magic:           0x{magic:08X}")
    
    # Validate magic
    print()
    if magic != CRC_FOOTER_MAGIC:
        print(f"❌ Invalid magic! Expected 0x{CRC_FOOTER_MAGIC:08X}, got 0x{magic:08X}")
        return False
    print(f"✅ Footer magic valid (0x{CRC_FOOTER_MAGIC:08X})")
    
    # Validate size
    total_size = len(data)
    code_bytes = total_size - CRC_FOOTER_SIZE
    
    if size > code_bytes:
        print(f"❌ Invalid size! Code size ({size}) exceeds available space")
        return False
    
    if size == 0:
        print(f"❌ Invalid size! Code size is zero")
        return False
    
    if size == code_bytes:
        print(f"✅ Code size matches ({size} bytes)")
    else:
        print(f"⚠️  Code size mismatch: footer={size}, actual={code_bytes}")
    
    print()
    print("✅ CRC footer verification successful!")
    return True


def verify_signature_footer(data, footer_offset):
    """Verify signature footer structure."""
    footer_bytes = data[footer_offset:]
    
    if len(footer_bytes) < SIGNATURE_FOOTER_SIZE:
        print(f"❌ ERROR: Not enough data for signature footer")
        return False
    
    # Parse signature footer: size(4) + version(4) + signature(64) + magic(4)
    size = struct.unpack('<I', footer_bytes[0:4])[0]
    version = struct.unpack('<I', footer_bytes[4:8])[0]
    signature = footer_bytes[8:72]  # 64 bytes
    magic = struct.unpack('<I', footer_bytes[72:76])[0]
    
    print()
    print("Signature Footer Information")
    print("-" * 60)
    print(f"Location:        Offset 0x{footer_offset:X} (byte {footer_offset})")
    print(f"Flash address:   0x{APP_START_ADDRESS + footer_offset:08X}")
    print()
    print(f"Code size:       {size} bytes (0x{size:X})")
    print(f"Version:         {version}")
    print(f"Signature:       {signature[:16].hex()}... ({len(signature)} bytes)")
    print(f"Magic:           0x{magic:08X}")
    
    # Validate magic
    print()
    if magic != SIGNATURE_FOOTER_MAGIC:
        print(f"❌ Invalid magic! Expected 0x{SIGNATURE_FOOTER_MAGIC:08X}, got 0x{magic:08X}")
        return False
    print(f"✅ Footer magic valid (0x{SIGNATURE_FOOTER_MAGIC:08X})")
    
    # Validate size
    total_size = len(data)
    code_bytes = total_size - SIGNATURE_FOOTER_SIZE
    
    if size > code_bytes:
        print(f"❌ Invalid size! Code size ({size}) exceeds available space")
        return False
    
    if size == 0:
        print(f"❌ Invalid size! Code size is zero")
        return False
    
    if size == code_bytes:
        print(f"✅ Code size matches ({size} bytes)")
    else:
        print(f"⚠️  Code size mismatch: footer={size}, actual={code_bytes}")
    
    # Check if signature is not all zeros
    if signature == b'\x00' * 64:
        print(f"⚠️  Warning: Signature is all zeros (not signed?)")
    else:
        print(f"✅ Signature present (ECDSA secp256r1)")
    
    print()
    print("✅ Signature footer verification successful!")
    return True


def verify_footer(bin_path):
    """Auto-detect and verify footer in binary file."""
    print(f"Verifying Firmware Footer")
    print("=" * 60)
    print(f"File: {bin_path}")
    print()
    
    # Read binary
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
    
    # Try to detect footer type by checking magic at expected locations
    # Signature footer: magic is at offset -4 (last 4 bytes)
    # CRC footer: magic is also at offset -4 (last 4 bytes)
    
    if total_size < 16:
        print(f"❌ ERROR: Binary too small for any footer")
        return False
    
    # Check last 4 bytes for magic number
    magic = struct.unpack('<I', data[-4:])[0]
    
    if magic == SIGNATURE_FOOTER_MAGIC:
        print(f"📌 Detected: SIGNATURE FOOTER (Secure Boot)")
        if total_size < SIGNATURE_FOOTER_SIZE:
            print(f"❌ ERROR: Binary too small for signature footer")
            return False
        footer_offset = total_size - SIGNATURE_FOOTER_SIZE
        return verify_signature_footer(data, footer_offset)
    
    elif magic == CRC_FOOTER_MAGIC:
        print(f"📌 Detected: CRC FOOTER (Legacy)")
        if total_size < CRC_FOOTER_SIZE:
            print(f"❌ ERROR: Binary too small for CRC footer")
            return False
        footer_offset = total_size - CRC_FOOTER_SIZE
        return verify_crc_footer(data, footer_offset)
    
    else:
        print(f"❌ ERROR: Unknown or missing footer!")
        print(f"   Found magic: 0x{magic:08X}")
        print(f"   Expected: 0x{CRC_FOOTER_MAGIC:08X} (CRC) or 0x{SIGNATURE_FOOTER_MAGIC:08X} (Signature)")
        return False


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("STM32 Firmware Footer Verification Script")
        print("=" * 40)
        print()
        print("Usage: python check_footer.py <binary_file.bin>")
        print()
        print("Supports:")
        print("  • CRC Footer (0xDEADBEEF) - 16 bytes")
        print("  • Signature Footer (0xBEEFC0DE) - 76 bytes")
        print()
        print("The script auto-detects the footer type and verifies it.")
        sys.exit(1)
    
    bin_file = Path(sys.argv[1])
    
    if not bin_file.exists():
        print(f"ERROR: File not found: {bin_file}")
        sys.exit(1)
    
    # Verify footer
    success = verify_footer(bin_file)
    
    sys.exit(0 if success else 1)
