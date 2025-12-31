# STM32F103RB Baremetal Project

A production-ready UART bootloader implementation for STM32F103RB with CRC-32 firmware integrity verification, built entirely from scratch without HAL/CMSIS libraries.

![STM32F103RB](https://img.shields.io/badge/STM32-F103RB-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Build](https://img.shields.io/badge/build-passing-brightgreen)

---

## 🎯 **Features**

### **UART Bootloader**
- ✅ Button-triggered bootloader mode (NUCLEO PC13)
- ✅ UART firmware flashing (115200 baud)
- ✅ Command protocol: `GO`, `ERASE`, `WRITE`, `READ`
- ✅ Auto-jump to application on boot
- ✅ 16KB bootloader footprint

### **CRC Integrity Verification**
- ✅ Hardware CRC-32 (STM32 peripheral, polynomial 0x04C11DB7)
- ✅ Dynamic footer scanning (no fixed address)
- ✅ Boot-time firmware validation
- ✅ Prevents corrupted firmware execution
- ✅ Backward compatible

### **Python Tooling**
- ✅ `inject_crc.py` - Automated CRC footer injection
- ✅ `check_footer.py` - Footer validation & inspection
- ✅ `inspect_binary.py` - Binary diagnostic tool
- ✅ `host_script.py` - Complete bootloader interface with serial monitor

### **Build System**
- ✅ Dual-target Makefile (bootloader + application)
- ✅ Automated CRC injection pipeline
- ✅ Convenience targets for flashing
- ✅ Size reporting and hex generation

---

## 📊 **Performance**

| Metric | Value |
|--------|-------|
| **Bootloader Size** | 4.6KB (of 16KB allocated) |
| **Application Binary** | 5.8KB (minimal, no padding) |
| **CRC Verification** | ~20μs (hardware accelerated) |
| **Footer Scanning** | 1-2ms (worst case) |
| **Total Boot Overhead** | <5ms |
| **UART Flash Speed** | ~210 bytes/sec |
| **UART Flash Time (6KB)** | ~28 seconds |

**vs. Padded Approach:**
- Binary size: **20x smaller** (5.8KB vs 114KB)
- UART flash: **20x faster** (28sec vs 9min)

---

## 🏗️ **Architecture**

### **Memory Map**
```
Flash (128KB):
  0x08000000 - 0x08004000 : Bootloader (16KB)
  0x08004000 - 0x08020000 : Application (112KB)

RAM (20KB):
  0x20000000 - 0x20005000 : Shared by bootloader/app
```

### **CRC Footer Structure**
```c
typedef struct {
    uint32_t size;      // Firmware code size
    uint32_t crc32;     // CRC-32 checksum
    uint32_t version;  
    uint32_t magic;     // 0xDEADBEEF signature
} __attribute__((packed)) CRC_Footer_t;  // 16 bytes
```

### **Boot Flow**
```
Power-On/Reset
      ↓
Check PC13 Button
      ↓
Pressed? → Bootloader Mode
      ↓
Not Pressed? → Scan for Footer
      ↓
Footer Found? → Verify CRC-32
      ↓
CRC Valid? → Jump to Application
      ↓
Invalid? → Stay in Bootloader
```

---

## 🚀 **Quick Start**

### **Prerequisites**
```bash
# Install ARM GCC Toolchain
sudo apt install gcc-arm-none-eabi

# Install Python dependencies
pip install pyserial

# Optional: STM32CubeProgrammer for ST-Link flashing
```

### **Building**
```bash
# Clone repository
git clone <repository-url>
cd STM32F103RB_Baremetal

# Build everything
make clean && make both

# Or build individually
make bl        # Bootloader only
make app       # Application only
```

### **Flashing**

**Option 1: ST-Link (Recommended for first time)**
```bash
make flash-bl      # Flash bootloader
make flash-app     # Flash application
# OR
make flash         # Flash both
```

**Option 2: UART Bootloader**
```bash
# 1. Hold PC13 button and reset board
# 2. Run host script
python bootloader/tools/host_script.py COM4

# 3. In the interactive shell:
> write build/application/app.bin
> monitor
```

---

## 📁 **Project Structure**

```
STM32F103RB_Baremetal/
├── bootloader/
│   ├── src/
│   │   ├── main.c           # Bootloader entry + CRC verification
│   │   ├── flash.c          # Flash programming routines
│   │   └── uart.c           # UART communication
│   ├── include/
│   └── linker/
│       └── STM32F103RBTX_BOOT.ld  # Bootloader linker (0x08000000)
├── application/
│   ├── src/
│   │   ├── main.c           # Application code
│   │   └── drivers/         # Peripheral drivers
│   ├── include/
│   └── linker/
│       └── STM32F103RBTX_APP.ld   # Application linker (0x08004000)
├── common/
│   ├── src/
│   │   ├── crc.c            # Hardware CRC driver
│   │   └── utility.c        # Shared utilities
│   └── include/
├── scripts/
│   ├── inject_crc.py        # CRC footer injection
│   ├── check_footer.py      # Footer verification
│   ├── inspect_binary.py    # Binary inspector
│   └── host_script.py       # Bootloader interface
├── Makefile                 # Build system
└── README.md
```

---

## 🔧 **Makefile Targets**

```bash
# Building
make bl              # Build bootloader
make app             # Build application
make both            # Build both
make clean           # Remove build artifacts

# Flashing (ST-Link)
make flash-bl        # Flash bootloader
make flash-app       # Flash application  
make flash           # Flash both

# Utilities
make size            # Show firmware sizes
make erase           # Erase flash
```

---

## 💻 **Usage Examples**

### **UART Bootloader Interface**
```bash
# Start host script
python bootloader/tools/host_script.py COM4

# Interactive commands:
> write app.bin      # Flash firmware
> read 0x08004000 64 # Read memory
> erase              # Erase application
> go                 # Jump to application
> monitor            # Serial monitor mode
```

### **CRC Footer Verification**
```bash
# Check footer in binary
python scripts/check_footer.py build/application/app.bin

# Output:
# ✅ Footer magic valid (0xDEADBEEF)
# ✅ Code size matches (5828 bytes)
# ✅ CRC-32: 0x9A843E61
```

### **Binary Inspection**
```bash
# Inspect binary structure
python scripts/inspect_binary.py build/application/app.bin

# Shows:
# - Vector table entries
# - CRC footer location
# - Memory layout
```

---

## 🧪 **Testing**

### **Hardware Setup**
- **Board:** STM32F103RB NUCLEO
- **Clock:** 72MHz (HSE + PLL)
- **UART:** USART2, PA2/PA3, 115200 baud
- **Button:** PC13 (USER button)
- **LED:** PA5 (LD2 green LED)

### **Test Scenarios**
```bash
# 1. Fresh flash (both bootloader + app)
make flash

# 2. UART firmware update
python bootloader/tools/host_script.py COM4
> write app.bin

# 3. CRC validation (should boot)
# Reset board → LED blinks

# 4. CRC rejection (corrupt firmware)
# Modify binary → bootloader refuses to boot
```

---

## 🔐 **Security**

### **Current Features**
- ✅ CRC-32 integrity verification (detects corruption)
- ✅ MSP validation (prevents invalid applications)
- ✅ Flash bounds checking
- ✅ Magic number validation

### **Future Enhancements**
- ⏭️ Ed25519 signature verification (secure boot)
- ⏭️ Firmware version management
- ⏭️ Dual-bank firmware (A/B updates)
- ⏭️ Encrypted firmware support

---

## 🎓 **Educational Value**

This project demonstrates:
- ✅ Bootloader design patterns
- ✅ Hardware peripheral usage (CRC, UART, Flash)
- ✅ Dynamic memory scanning algorithms
- ✅ Build system automation
- ✅ Python-C integration
- ✅ Production-grade embedded practices

---

## 🛠️ **Development**

### **Adding New Features**
```bash
# 1. Modify application code
vim application/src/main.c

# 2. Rebuild
make clean && make app

# 3. Flash via UART
python bootloader/tools/host_script.py COM4
> write app.bin
```

### **Debugging**
```bash
# View serial logs
python bootloader/tools/host_script.py COM4
> monitor

# Expected bootloader output:
# [LOG] CRC footer found. Verifying...
# [LOG] CRC OK!
# [LOG] Jumping to App...
```

---

## ⚙️ **Configuration**

### **Bootloader Settings** (`bootloader/src/main.c`)
```c
#define BL_DEBUG 1           // Enable debug logs
#define FLASH_START_ADDRESS  0x08004000  // App start
```

### **Application Settings** (`application/linker/STM32F103RBTX_APP.ld`)
```ld
FLASH (rx) : ORIGIN = 0x08004000, LENGTH = 112K
```

### **Build Settings** (`Makefile`)
```makefile
OPT = -O2                    # Optimization level
```

---

## 🐛 **Troubleshooting**

| Issue | Solution |
|-------|----------|
| **Build fails** | Run `make clean` first |
| **Flash fails** | Check ST-Link connection |
| **UART not working** | Verify COM port and baud rate |
| **CRC fails** | Rebuild app with `make clean && make app` |
| **App doesn't boot** | Check VTOR setting: `SCB->VTOR = 0x08004000` |

---

## 📝 **License**

This project is open source.

---

## 👤 **Author**

Created as part of STM32 baremetal embedded systems learning journey.

---

## 🙏 **Acknowledgments**

- STM32F103 Reference Manual (RM0008)
- ARM Cortex-M3 Technical Reference Manual
- Community feedback and testing

---

## 📬 **Contact**

For questions, issues, or contributions, please open an issue on GitHub.

---

**Happy Coding! 🚀**
