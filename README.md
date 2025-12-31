# STM32F103RB Baremetal Project

A comprehensive baremetal firmware project for STM32F103RB microcontroller, featuring:
- **Custom peripheral drivers** written from scratch (no HAL/LL)
- **UART bootloader** with CRC-32 integrity verification
- **Windows development environment** using ARM GNU Toolchain

---

## 🎯 **Project Goals**

This project aims to:
1. **Develop baremetal drivers** for STM32F103RB peripherals (GPIO, UART, SPI, I2C, ADC, Timers, etc.)
2. **Implement a production-ready UART bootloader** with firmware verification
3. **Learn embedded systems** through direct register manipulation (no abstraction layers)
4. **Build entirely on Windows** using GCC ARM toolchain and Make

---

## 🛠️ **Development Environment (Windows)**

### **Required Tools**

#### **1. ARM GNU Toolchain**
Download and install from:
- [ARM GNU Toolchain Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain)
- Choose: **arm-none-eabi** (bare-metal target)
- Add to PATH: `C:\ST\STM32CubeCLT_1.19.0\GNU-tools-for-STM32\bin`

**Verify installation:**
```powershell
arm-none-eabi-gcc --version
```

#### **2. Make for Windows**
Install using one of:
- **MinGW** - [mingw-w64.org](https://www.mingw-w64.org/)
- **MSYS2** - [msys2.org](https://www.msys2.org/)
- **Chocolatey** - `choco install make`

**Verify installation:**
```powershell
make --version
```

#### **3. Python 3**
For build scripts and UART bootloader interface:
- [Python Downloads](https://www.python.org/downloads/)
- Add to PATH during installation

**Install dependencies:**
```powershell
pip install pyserial
```

#### **4. STM32CubeProgrammer** (Optional but recommended)
For ST-Link flashing:
- [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)
- Provides `STM32_Programmer_CLI` command-line tool

---

## 📁 **Project Structure**

```
STM32F103RB_Baremetal/
├── bootloader/                 # UART Bootloader (16KB)
│   ├── src/
│   │   ├── main.c             # Entry point, CRC verification, jump logic
│   │   ├── flash.c            # Flash programming routines
│   │   └── uart.c             # UART communication
│   ├── include/
│   ├── linker/
│   │   └── STM32F103RBTX_BOOT.ld   # Linker script (0x08000000)
│   └── tools/
│       └── host_script.py     # PC-side bootloader interface
│
├── application/                # Main Application (112KB)
│   ├── src/
│   │   ├── main.c             # Application entry point
│   │   └── drivers/           # Baremetal peripheral drivers
│   │       ├── gpio.c
│   │       ├── uart.c
│   │       ├── spi.c
│   │       ├── i2c.c
│   │       ├── adc.c
│   │       ├── timer.c
│   │       ├── rtc.c
│   │       ├── watchdog.c
│   │       └── ... (more to come)
│   ├── include/
│   └── linker/
│       └── STM32F103RBTX_APP.ld    # Linker script (0x08004000)
│
├── common/                     # Shared code
│   ├── src/
│   │   ├── crc.c              # Hardware CRC-32 driver
│   │   ├── utility.c          # LED, printf helpers
│   │   └── system_stm32f1xx.c # Clock initialization
│   └── include/
│
├── scripts/                    # Build & verification tools
│   ├── inject_crc.py          # Append CRC footer to binary
│   ├── check_footer.py        # Verify CRC footer
│   └── inspect_binary.py      # Binary diagnostic tool
│
├── Makefile                    # Build system
└── README.md
```

---

## 🏗️ **Bootloader Features**

The bootloader provides:
- **UART firmware flashing** (115200 baud, USART2)
- **CRC-32 integrity checking** using STM32 hardware peripheral
- **Dynamic footer scanning** (no fixed firmware size)
- **Button-triggered mode** (PC13 button on NUCLEO board)
- **Auto-boot** to application if button not pressed
- **Backward compatible** with firmware lacking CRC footer

**Memory Layout:**
```
Flash (128KB):
  0x08000000 - 0x08004000 : Bootloader (16KB)
  0x08004000 - 0x08020000 : Application (112KB)
```

---

## 🚀 **Quick Start Guide**

### **1. Clone Repository**
```powershell
git clone <repository-url>
cd STM32F103RB_Baremetal
```

### **2. Build Project**
```powershell
# Clean previous builds
make clean

# Build bootloader only
make bl

# Build application only
make app

# Build both
make both
```

**Build output:**
- `build/bootloader/bootloader.bin` - Bootloader binary
- `build/application/app.bin` - Application binary (with CRC footer)

### **3. Flash to Board**

**Option A: ST-Link (First-time setup)**
```powershell
# Flash bootloader
make flash-bl

# Flash application
make flash-app

# Or flash both at once
make flash
```

**Option B: UART Bootloader (After initial setup)**
```powershell
# 1. Hold PC13 button and reset board
# 2. Open bootloader interface
python bootloader/tools/host_script.py COM4

# 3. Flash firmware
> write build/application/app.bin

# 4. Monitor serial output
> monitor
```

---

## 📚 **Peripheral Drivers (Baremetal)**

All drivers are written from scratch using direct register access:

| Peripheral | Status | Features |
|------------|--------|----------|
| **GPIO** | ✅ Complete | Pin config, read/write, interrupts |
| **UART** | ✅ Complete | TX/RX, DMA support, printf |
| **CRC** | ✅ Complete | Hardware CRC-32 (bootloader) |
| **Flash** | ✅ Complete | Erase, program, read |
| **SPI** | ✅ Complete | Master mode, DMA |
| **I2C** | ✅ Complete | Master/slave, 7/10-bit addressing |
| **ADC** | ✅ Complete | Single/continuous conversion |
| **Timer** | ✅ Complete | PWM, input capture, interrupts |
| **RTC** | ✅ Complete | Date/time, alarm, backup registers |
| **Watchdog** | ✅ Complete | Independent watchdog (IWDG) |
| **SysTick** | ✅ Complete | Delays, timekeeping |
| **Display** | ✅ Complete | ST7789 LCD driver (SPI-based) |

**Note:** All drivers follow a consistent API design for ease of use.

---

## 💻 **Makefile Targets**

```powershell
# Building
make bl              # Build bootloader
make app             # Build application
make both            # Build both (default)
make clean           # Remove all build artifacts

# Flashing (requires ST-Link)
make flash-bl        # Flash bootloader
make flash-app       # Flash application
make flash           # Flash both

# Utilities
make size            # Display firmware sizes
make erase           # Erase entire flash
```

---

## 🔧 **Workspace Setup (Windows)**

### **Step 1: Install Tools**
1. Install ARM GNU Toolchain (add to PATH)
2. Install Make (MinGW/MSYS2)
3. Install Python 3 + pip
4. Install STM32CubeProgrammer

### **Step 2: Verify Installation**
```powershell
arm-none-eabi-gcc --version
make --version
python --version
STM32_Programmer_CLI --version
```

### **Step 3: Clone and Build**
```powershell
git clone <repo-url>
cd STM32F103RB_Baremetal
make both
```

### **Step 4: Connect Hardware**
- Connect NUCLEO-F103RB via USB (ST-Link V2)
- Windows should recognize ST-Link driver
- Note COM port number (Device Manager → Ports)

### **Step 5: Flash**
```powershell
make flash
```

---

## 🧪 **Testing**

### **Hardware Required**
- **NUCLEO-F103RB** development board
- USB cable (ST-Link)
- Serial terminal (PuTTY, Tera Term, or host_script.py)

### **Quick Test**
1. Flash bootloader and application: `make flash`
2. Reset board (don't hold button)
3. LED on PA5 should blink
4. Open serial monitor:
   ```powershell
   python bootloader/tools/host_script.py COM4
   > monitor
   ```
5. You should see application logs

---

## 🔍 **Development Workflow**

### **Typical Development Cycle**

1. **Modify application code**
   ```powershell
   notepad application\src\main.c
   ```

2. **Rebuild**
   ```powershell
   make clean && make app
   ```

3. **Flash via UART**
   ```powershell
   python bootloader\tools\host_script.py COM4
   > write build\application\app.bin
   > monitor
   ```

### **Adding New Peripheral Driver**

1. Create `application/src/drivers/new_peripheral.c`
2. Create `application/include/new_peripheral.h`
3. Add to Makefile `C_SOURCES`
4. Implement init, read, write functions
5. Test and document

---

## 📖 **Learning Resources**

- **STM32F103 Reference Manual** (RM0008) - Peripheral register details
- **Cortex-M3 Technical Reference** - ARM architecture
- **Datasheets** - Pin configurations, electrical characteristics
- **Code Comments** - All drivers have detailed inline documentation

---

## 🎓 **Educational Value**

This project teaches:
- ✅ Direct register manipulation (no HAL abstraction)
- ✅ Linker scripts and memory layout
- ✅ Startup code and vector tables
- ✅ Bootloader design patterns
- ✅ Build system automation (Makefiles)
- ✅ Python-C integration
- ✅ Firmware integrity verification

---

## ⚙️ **Configuration**

### **Bootloader Configuration** (`bootloader/src/main.c`)
```c
#define BL_DEBUG 1           // Enable/disable debug logs
#define APP_START 0x08004000 // Application start address
```

### **System Clock** (`common/src/system_stm32f1xx.c`)
```c
SystemCoreClock = 72000000;  // 72MHz (HSE + PLL)
```

### **UART Settings** (`bootloader/src/uart.c`)
```c
Baud Rate: 115200
Data bits: 8
Stop bits: 1
Parity: None
```

---

## 🐛 **Troubleshooting**

| Issue | Solution |
|-------|----------|
| `make: command not found` | Install Make (MinGW/MSYS2), add to PATH |
| `arm-none-eabi-gcc: not found` | Install toolchain, verify PATH |
| ST-Link not detected | Install STM32CubeProgrammer drivers |
| COM port not found | Check Device Manager, install USB serial drivers |
| Build fails | Run `make clean` first |
| Flash fails | Try `make erase` then `make flash` |
| CRC mismatch | Rebuild: `make clean && make both` |
| App doesn't boot | Verify VTOR: `SCB->VTOR = 0x08004000` in app code |

---

## 📝 **Notes**

- All code is **Windows-native** (PowerShell commands, Windows paths)
- Drivers use **CMSIS headers** only (no HAL/LL)
- CRC footer is **automatically injected** during build (`inject_crc.py`)
- Bootloader **verifies firmware** on every boot (<5ms overhead)
- Project follows **educational best practices** (comments, clean code)

---

## 🎯 **Future Plans**

- [ ] Add secure boot (Ed25519 signature verification)
- [ ] Implement DMA for all peripherals
- [ ] Add low-power modes
- [ ] Create unit tests for drivers
- [ ] Port to other STM32 variants
- [ ] Add Ethernet driver
- [ ] Implement USB CDC

---

## 👤 **Author**

Created as part of learning embedded systems development on Windows.

---

## 📬 **Contributing**

Contributions welcome! Feel free to:
- Report bugs
- Suggest driver improvements
- Add new peripheral drivers
- Improve documentation

---

**Happy Embedded Coding! 🚀**
