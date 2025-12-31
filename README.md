# STM32F103RB Bare-Metal Project

A **bare-metal firmware project** for STM32F103RB (Cortex-M3) demonstrating driver development and advanced features **without HAL/LL libraries**.

## 🎯 Project Goals

- ✅ Bare-metal driver implementation (UART, CRC, Flash, GPIO)
- ✅ **Secure Boot** with ECDSA signature verification
- ✅ UART bootloader for firmware updates
- ✅ Cryptographic libraries (SHA-256, ECDSA secp256r1)
- ✅ Production-ready memory layout with bootloader/application separation

## 🏗️ Architecture

### Memory Layout
```
Flash (128KB):
├── Bootloader: 0x08000000 - 0x08007FFF (32KB)
│   └── Secure boot verification + UART update capability
└── Application: 0x08008000 - 0x0801FFFF (96KB)
    └── User firmware + 76-byte signature footer
```

### Project Structure
```
STM32F103RB_Baremetal/
├── bootloader/          # Secure bootloader (32KB)
│   ├── src/            # Boot logic, signature verification
│   ├── include/        # Flash, crypto headers
│   └── linker/         # Bootloader linker script
├── application/         # User application (96KB)
│   ├── src/            # Application code
│   ├── include/        # App headers
│   └── linker/         # Application linker script
├── common/              # Shared bare-metal drivers
│   ├── src/            # UART, CRC, SHA256, crypto wrapper
│   ├── include/        # Driver headers
│   └── crypto/
│       └── micro-ecc/  # ECDSA library (reorganized)
│           ├── include/    # uECC headers
│           ├── src/        # Crypto implementation
│           └── asm/        # ARM optimizations
├── scripts/             # Build automation
│   ├── generate_keys.py    # ECDSA key pair generation
│   ├── sign_firmware.py    # Firmware signing
│   └── host_script.py      # UART flash tool
└── keys/                # Cryptographic keys
    ├── private_key.pem     # Private signing key
    ├── public_key.pem      # Public verification key
    └── public_key.h        # Embedded in bootloader
```

## 🔐 Implemented Features

### 1. Secure Boot
- **ECDSA secp256r1** signature verification using micro-ecc
- **SHA-256** firmware hashing
- **Fail-closed** security (refuses unsigned/tampered firmware)
- Automated signing during build

### 2. UART Bootloader
- Interactive shell for firmware updates
- Commands: `erase`, `flash`, `read`, `jump`, `monitor`
- Supports firmware update over serial (115200 baud)

### 3. Bare-Metal Drivers
- **UART**: Serial communication (blocking I/O)
- **CRC**: Hardware CRC-32 peripheral
- **Flash**: Flash programming and erase operations
- **SHA-256**: Software cryptographic hash
- **Crypto wrapper**: ECDSA signature verification

## 🛠️ Setup & Dependencies

### Prerequisites
```bash
# ARM Toolchain
- arm-none-eabi-gcc
- arm-none-eabi-gdb
- arm-none-eabi-size
- arm-none-eabi-objcopy

# Flashing Tool
- STM32CubeProgrammer (provides STM32_Programmer_CLI)

# Python (for signing and UART tools)
pip install cryptography
```

## 🚀 Quick Start

### 1. Generate Cryptographic Keys (One-time)
```bash
make genkeys
```
Generates ECDSA secp256r1 key pair in `keys/`

### 2. Build Both Targets
```bash
make clean
make both
```
This builds:
- `build/bootloader/bootloader.bin` (32KB)
- `build/application/app.bin` (auto-signed with ECDSA)

### 3. Flash via ST-LINK
```bash
# Erase chip
make erase

# Flash bootloader
make flash-bl

# Flash signed application (via UART bootloader)
python scripts/host_script.py COM4
> flash build/application/app.bin
```

### 4. Verify Secure Boot
Connect serial monitor (115200 baud):
```
[LOG] Bootloader Active v1.0
[LOG] Signature found. Verifying...
[LOG] Signature VALID - Booting...
we are in application
```

## � Makefile Targets

### Build
```bash
make bl          # Build bootloader only
make app         # Build and auto-sign application
make both        # Build both targets
make clean       # Clean build artifacts
```

### Flash (ST-LINK)
```bash
make erase       # Full chip erase
make flash-bl    # Flash bootloader at 0x08000000
make flash-app   # Flash application at 0x08008000 (requires bootloader)
make flash-both  # Flash both in sequence
```

### Security
```bash
make genkeys     # Generate ECDSA key pair
make sign        # Manually sign application binary
```

## 🔧 UART Bootloader Usage

### Python Flash Tool
```bash
python scripts/host_script.py COM4
```

### Interactive Commands
```
(STM32-BL) > help
Commands:
  erase        - Erase application region
  flash <file> - Erase, write, and jump to application
  read <addr> <len> - Read memory
  jump         - Jump to application
  monitor      - Serial monitor mode (Ctrl+C to quit)
  help         - Show this list
  exit         - Quit shell
```

### Example: Flash Firmware
```
(STM32-BL) > flash build/application/app.bin
Erasing Application Region...
[LOG] Result: OK
Writing: |████████████████████████|  100% Complete
Flashing Complete!
Jumping to Application...
```

## � Technical Details

### Secure Boot Process
1. **Check signature footer** (magic: `0xBEEFC0DE`)
2. **Calculate SHA-256** hash of firmware
3. **Verify ECDSA signature** with embedded public key
4. ✅ **Valid** → Boot | ❌ **Invalid** → Refuse (fail-closed)

### Signature Footer (76 bytes)
```c
typedef struct {
    uint32_t firmware_size;  // 4 bytes
    uint32_t version;        // 4 bytes
    uint8_t  signature[64];  // 64 bytes (ECDSA R||S)
    uint32_t magic;          // 4 bytes (0xBEEFC0DE)
} __attribute__((packed)) SignatureFooter_t;
```

### Cryptography Stack
- **Algorithm**: ECDSA secp256r1 (NIST P-256)
- **Hash**: SHA-256 (FIPS 180-4 compliant)
- **Library**: micro-ecc (optimized for embedded ARM)
- **Verification Time**: ~300-400ms

## 🧪 Development Workflow

### Modify Application
```bash
# 1. Edit application code
nano application/src/main.c

# 2. Build and auto-sign
make app

# 3. Flash via UART bootloader
python scripts/host_script.py COM4
> flash build/application/app.bin
```

### Update Bootloader
```bash
# 1. Edit bootloader
nano bootloader/src/main.c

# 2. Build
make bl

# 3. Flash via ST-LINK (requires SWD)
make flash-bl
```

## ⚠️ Recent Fixes (2025-12-31)

### Critical Address Bug Fixed
- **Issue**: Bootloader expanded to 32KB but constants still used 16KB offset
- **Fixed**: `scripts/host_script.py` and `bootloader/include/flash.h`
- **Result**: UART flashing now works correctly

### Micro-ECC Library Reorganized
- Cleaned up flat directory into `include/`, `src/`, `asm/` structure
- Removed test/example code (12 files vs 17 + 3 dirs)
- Updated Makefile paths

## 📚 Learning Resources

This project demonstrates:
- Bare-metal ARM Cortex-M3 programming
- Bootloader design and implementation
- Cryptographic signature verification
- Flash memory management
- UART protocol implementation
- Build system configuration (Makefile, linker scripts)

## 🔒 Security Notes

**Current Implementation**:
- ✅ Firmware authentication via ECDSA
- ✅ Tamper detection (signature invalidation)
- ✅ Fail-closed security model

**Limitations**:
- ⚠️ No rollback protection
- ⚠️ No secure key storage (software-based)
- ⚠️ Debug ports accessible

**Production Recommendations**:
- Enable Read Protection Level 1
- Disable debug ports
- Implement version-based rollback protection

## 📝 License

Educational and commercial use permitted.

---

**Platform**: STM32F103RB (NUCLEO-F103RB)  
**Toolchain**: ARM GCC, STM32CubeProgrammer  
**Status**: ✅ Fully functional with secure boot
