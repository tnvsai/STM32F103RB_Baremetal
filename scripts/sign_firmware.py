#!/usr/bin/env python3
"""
Sign Firmware with ECDSA Signature for Secure Boot

Usage: python sign_firmware.py <firmware.bin>
"""

import sys
import os
import struct
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature, Prehashed

SIGNATURE_MAGIC = 0xBEEFC0DE

def calculate_sha256(data):
    digest = hashes.Hash(hashes.SHA256(), backend=default_backend())
    digest.update(data)
    return digest.finalize()

def der_to_raw_signature(der_signature):
    """Convert DER signature to raw R||S (big-endian for micro-ecc)"""
    r, s = decode_dss_signature(der_signature)
    r_bytes = r.to_bytes(32, byteorder='big')
    s_bytes = s.to_bytes(32, byteorder='big')
    return r_bytes + s_bytes

def create_signature_footer(firmware_size, version, signature):
    """Create 76-byte signature footer"""
    footer = b''
    footer += struct.pack('<I', firmware_size)
    footer += struct.pack('<I', version)
    footer += signature
    footer += struct.pack('<I', SIGNATURE_MAGIC)
    assert len(footer) == 76, f"Footer size mismatch: {len(footer)} bytes"
    return footer

def sign_firmware(firmware_path, private_key, version=1):
    print(f"[*] Loading firmware: {firmware_path}")
    with open(firmware_path, 'rb') as f:
        firmware_data = f.read()
    
    firmware_size = len(firmware_data)
    print(f"    Firmware size: {firmware_size} bytes ({firmware_size/1024:.2f} KB)")
    
    # Calculate SHA-256
    print("[*] Calculating SHA-256 hash...")
    plain_hash = calculate_sha256(firmware_data)
    print(f"    Hash: {plain_hash.hex()}")
    
    # Sign with Prehashed mode (CRITICAL FIX!)
    print("[*] Signing with ECDSA (Prehashed mode)...")
    der_signature = private_key.sign(plain_hash, ec.ECDSA(Prehashed(hashes.SHA256())))
    raw_signature = der_to_raw_signature(der_signature)
    print(f"    Signature: {raw_signature.hex()[:32]}...")
    
    # Create footer
    print("[*] Creating signature footer...")
    footer = create_signature_footer(firmware_size, version, raw_signature)
    
    # Append to firmware
    signed_firmware = firmware_data + footer
    print(f"[*] Total size: {len(signed_firmware)} bytes (firmware + 76B footer)")
    
    # Write signed firmware
    with open(firmware_path, 'wb') as f:
        f.write(signed_firmware)
    
    print(f"[+] Firmware signed successfully!")
    return True

def load_private_key(key_path):
    with open(key_path, 'rb') as f:
        private_key = serialization.load_pem_private_key(
            f.read(), password=None, backend=default_backend()
        )
    return private_key

def remove_old_signature(firmware_path):
    """Remove existing signature if present"""
    with open(firmware_path, 'rb') as f:
        data = f.read()
    
    if len(data) >= 76:
        magic = struct.unpack('<I', data[-4:])[0]
        if magic == SIGNATURE_MAGIC:
            print("[*] Removing old signature...")
            data = data[:-76]
            with open(firmware_path, 'wb') as f:
                f.write(data)
            return True
    return False

def main():
    if len(sys.argv) < 2:
        print("Usage: python sign_firmware.py <firmware.bin>")
        sys.exit(1)
    
    firmware_path = sys.argv[1]
    if not os.path.exists(firmware_path):
        print(f"Error: {firmware_path} not found")
        sys.exit(1)
    
    key_path = 'keys/private_key.pem'
    if not os.path.exists(key_path):
        print(f"Error: {key_path} not found")
        print("Run: make genkeys")
        sys.exit(1)
    
    remove_old_signature(firmware_path)
    
    print("="*60)
    print("Firmware Signing Tool")
    print("="*60)
    
    private_key = load_private_key(key_path)
    success = sign_firmware(firmware_path, private_key)
    
    if success:
        print("\n✅ Firmware ready for secure boot!")
    else:
        print("\n❌ Signing failed")
        sys.exit(1)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"\n❌ Error: {e}")
        sys.exit(1)
