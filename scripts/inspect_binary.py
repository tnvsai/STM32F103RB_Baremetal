#!/usr/bin/env python3
"""
Binary Inspector for STM32 Application
=======================================

Displays key information about the compiled STM32 application binary.
This is a diagnostic tool to verify the binary structure after compilation.

WHAT IT SHOWS:
--------------
- Total binary size
- Vector table entries (MSP, Reset Handler)
- CRC footer location and contents (if present)

USAGE:
------
    python inspect_binary.py [binary_file.bin]

If no file is specified, defaults to 'build/application/app.bin'
"""

import sys
import struct
from pathlib import Path

# Footer constants
FOOTER_SIZE = 16
FOOTER_MAGIC = 0xDEADBEEF


def inspect_binary(bin_path):
    """
    Display detailed information about the binary file.
    
    Args:
        bin_path (str): Path to binary file to inspect
    """
    print("=" * 70)
    print("STM32 Application Binary Inspector")
    print("=" * 70)
    print(f"File: {bin_path}\n")
    
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
    print(f"Binary Size: {total_size} bytes (0x{total_size:X})\n")
    
    # Check minimum size
    if total_size < 8:
        print("❌ Binary too small (must be at least 8 bytes)")
        return False
    
    # Display Vector Table (first 8 bytes)
    print("Vector Table (First 8 Bytes)")
    print("-" * 70)
    msp = struct.unpack('<I', data[0x00:0x04])[0]
    reset = struct.unpack('<I', data[0x04:0x08])[0]
    
    print(f"  MSP (0x00):          0x{msp:08X}")
    print(f"  Reset Handler (0x04): 0x{reset:08X}")
    
    # Validate MSP
    if (msp & 0x2FF00000) == 0x20000000:
        print(f"  ✅ MSP valid (points to RAM)\n")
    else:
        print(f"  ⚠️  MSP invalid (not in RAM range)\n")
    
    # Check for CRC Footer
    print("CRC Footer Analysis")
    print("-" * 70)
    
    if total_size >= FOOTER_SIZE:
        footer_offset = total_size - FOOTER_SIZE
        footer_bytes = data[footer_offset:]
        
        try:
            size, crc32, version, magic = struct.unpack('<IIII', footer_bytes)
            
            print(f"  Location: Offset 0x{footer_offset:X} (last 16 bytes)")
            print(f"  Size:     {size} bytes (0x{size:X})")
            print(f"  CRC32:    0x{crc32:08X}")
            print(f"  Version:  {version}")
            print(f"  Magic:    0x{magic:08X}")
            
            if magic == FOOTER_MAGIC:
                print(f"\n  ✅ Valid CRC footer detected!")
                
                # Verify size makes sense
                if size > 0 and size <= total_size - FOOTER_SIZE:
                    print(f"  ✅ Size field valid ({size} bytes of code)")
                else:
                    print(f"  ⚠️  Size field suspicious ({size} bytes)")
            else:
                print(f"\n  ❌ No valid footer (expected magic: 0x{FOOTER_MAGIC:08X})")
        except struct.error:
            print("  ❌ Cannot parse footer")
    else:
        print("  ❌ Binary too small to contain footer")
    
    print("\n" + "=" * 70)
    return True


if __name__ == "__main__":
    # Default to application binary if no argument provided
    if len(sys.argv) > 1:
        bin_file = Path(sys.argv[1])
    else:
        bin_file = Path("build/application/app.bin")
    
    # Check file exists
    if not bin_file.exists():
        print(f"ERROR: File not found: {bin_file}")
        print()
        print("Usage: python inspect_binary.py [binary_file.bin]")
        sys.exit(1)
    
    # Inspect the binary
    success = inspect_binary(bin_file)
    sys.exit(0 if success else 1)
