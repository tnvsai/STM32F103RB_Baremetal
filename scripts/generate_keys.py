#!/usr/bin/env python3
"""
Generate ECDSA secp256r1 Key Pair for Secure Boot

Usage: python generate_keys.py
Output:
    keys/private_key.pem  - Private key (KEEP SECURE!)
    keys/public_key.pem   - Public key
    keys/public_key.h     - C header for bootloader
"""

import os
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend

def generate_keypair():
    print("[*] Generating ECDSA secp256r1 key pair...")
    private_key = ec.generate_private_key(ec.SECP256R1(), default_backend())
    public_key = private_key.public_key()
    return private_key, public_key

def save_private_key(private_key, filepath):
    pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    )
    with open(filepath, 'wb') as f:
        f.write(pem)
    print(f"[+] Private key saved to: {filepath}")
    print("    ⚠️  KEEP THIS SECURE! DO NOT COMMIT!")

def save_public_key(public_key, filepath):
    pem = public_key.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo
    )
    with open(filepath, 'wb') as f:
        f.write(pem)
    print(f"[+] Public key saved to: {filepath}")

def export_public_key_c_header(public_key, filepath):
    pub_numbers = public_key.public_numbers()
    x_bytes = pub_numbers.x.to_bytes(32, byteorder='big')
    y_bytes = pub_numbers.y.to_bytes(32, byteorder='big')
    public_key_bytes = x_bytes + y_bytes
    
    header = "/*\n"
    header += " * ECDSA secp256r1 Public Key for Secure Boot\n"
    header += " * Auto-generated - DO NOT MODIFY!\n"
    header += " */\n\n"
    header += "#ifndef PUBLIC_KEY_H\n"
    header += "#define PUBLIC_KEY_H\n\n"
    header += "#include <stdint.h>\n\n"
    header += "// ECDSA secp256r1 Public Key (64 bytes: X||Y)\n"
    header += "const uint8_t PUBLIC_KEY[64] = {\n"
    
    for i in range(0, 64, 16):
        chunk = public_key_bytes[i:i+16]
        hex_values = ', '.join(f'0x{b:02X}' for b in chunk)
        if i == 0:
            header += f"    // X coordinate\n"
        elif i == 32:
            header += f"\n    // Y coordinate\n"
        header += f"    {hex_values}"
        if i + 16 < 64:
            header += ",\n"
        else:
            header += "\n"
    
    header += "};\n\n"
    header += "#endif // PUBLIC_KEY_H\n"
    
    with open(filepath, 'w') as f:
        f.write(header)
    print(f"[+] C header saved to: {filepath}")

def update_gitignore(gitignore_path):
    entries_to_add = [
        "\n# Secure Boot Keys - DO NOT COMMIT PRIVATE KEYS\n",
        "keys/private_key.pem\n",
        "keys/*.key\n",
        "!keys/public_key.h\n",
        "!keys/public_key.pem\n"
    ]
    
    if os.path.exists(gitignore_path):
        with open(gitignore_path, 'r') as f:
            content = f.read()
        if 'private_key.pem' in content:
            print("[*] .gitignore already protected")
            return
    
    with open(gitignore_path, 'a') as f:
        for entry in entries_to_add:
            f.write(entry)
    print(f"[+] Updated {gitignore_path}")

def main():
    keys_dir = 'keys'
    os.makedirs(keys_dir, exist_ok=True)
    
    priv_key_path = os.path.join(keys_dir, 'private_key.pem')
    if os.path.exists(priv_key_path):
        response = input("⚠️  Keys exist! Overwrite? (yes/no): ")
        if response.lower() != 'yes':
            print("[!] Aborted.")
            return
    
    private_key, public_key = generate_keypair()
    save_private_key(private_key, priv_key_path)
    save_public_key(public_key, os.path.join(keys_dir, 'public_key.pem'))
    export_public_key_c_header(public_key, os.path.join(keys_dir, 'public_key.h'))
    update_gitignore('.gitignore')
    
    print("\n" + "="*60)
    print("✅ Key generation complete!")
    print("="*60)
    print("\n📌 Next steps:")
    print("  1. Rebuild bootloader with new public key")
    print("  2. Sign firmware with sign_firmware.py")
    print("  3. Flash to device\n")

if __name__ == "__main__":
    try:
        main()
    except ImportError as e:
        print(f"\n❌ Error: {e}")
        print("\n📦 Install: pip install cryptography\n")
    except Exception as e:
        print(f"\n❌ Error: {e}")
