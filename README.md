# STM32F103RB Bare-Metal

This project demonstrates a **bare-metal firmware** for the STM32F103RB (Cortex-M3) MCU using:
- Startup assembly (`startup_stm32f103rbtx.s`)
- Linker script (`STM32F103RBTX_FLASH.ld`)
- CMSIS system initialization (`system_stm32f1xx.c`)
- Minimal `syscalls.c` (newlib stubs)
- 

No HAL/LL libraries are used — only CMSIS and direct register access.

---

## 📂 Project Structure

```
STM32F103RB_Baremetal/
├── Makefile                  # Build rules
├── STM32F103RBTX_FLASH.ld    # Linker script
├── startup_stm32f103rbtx.s   # Startup (vector table + Reset_Handler)
├── main.c                    # Application (LED blink)
├── system_stm32f1xx.c        # CMSIS system clock initialization
├── syscalls.c                # Minimal stubs for newlib
├── include/
│   └── system_stm32f1xx.h    # CMSIS header
├── build/                    # Output binaries (after build)
├── STM32F103.svd             # Peripheral description (for debugging)
└── .vscode/                  # VS Code configs (launch.json, tasks.json)
```

---

## ⚙️ Dependencies

### 1. Toolchain
Install **Arm GNU Toolchain**:  
[Arm GNU Toolchain Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain)  

You need:
- `arm-none-eabi-gcc`
- `arm-none-eabi-gdb`
- `arm-none-eabi-size`
- `arm-none-eabi-objcopy`

---

### 2. Flashing Tool
Install **STM32CubeProgrammer**:  
[STM32CubeProgrammer Download](https://www.st.com/en/development-tools/stm32cubeprog.html)  

This provides the `STM32_Programmer_CLI` utility.

---

### 3. Debugging Tool
Install **OpenOCD** (xPack build recommended):  
[xPack OpenOCD](https://xpack.github.io/openocd/)  

Example path:  
```
C:\Program Files\xpack-openocd-0.12.0-6\bin\openocd.exe
```

---

### 4. VS Code Extensions
- [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug)  
- [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)  

---

## 🛠️ Build

From project root:

```powershell
make clean
make
```

This generates:
- `build/main.elf` → ELF (for debugging)
- `build/main.bin` → Binary (for flashing)

---

## 🔥 Flashing the MCU

Make sure your **NUCLEO-F103RB** is connected via ST-LINK.  

Flash the program:

```powershell
STM32_Programmer_CLI -c port=SWD -d build/main.bin 0x08000000 -rst
```

If flash erase errors occur (e.g., **RDP lock**), reset protection:

```powershell
STM32_Programmer_CLI -c port=SWD mode=UR -hardRst -ob RDP=0
```

---

## 🐞 Debugging in VS Code

1. Copy `STM32F103.svd` into project root.  
   This enables register/peripheral view.  

2. Create `.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Cortex Debug",
            "type": "cortex-debug",
            "request": "launch",
            "executable": "${workspaceFolder}/build/main.elf",
            "servertype": "openocd",
            "device": "STM32F103RB",
            "configFiles": [
                "interface/stlink.cfg",
                "target/stm32f1x.cfg"
            ],
            "svdFile": "${workspaceFolder}/STM32F103.svd",
            "runToMain": true,
            "cwd": "${workspaceFolder}"
        }
    ]
}
```

3. Start Debugging (`Run and Debug → Cortex Debug`).  
   - Breakpoints now work.  
   - Registers and peripherals are visible from `.svd`.  
   - You can add memory views.

---

## 🔍 Inspecting Memory & Registers

### In VS Code (Cortex-Debug)
- **Registers:**  
  Debug Panel → *Registers* → Expand peripherals.  

- **Memory:**  
  Command Palette → `Cortex-Debug: Add Memory View`.  
  Common addresses:
  - `0x20000000` → SRAM  
  - `0x08000000` → Flash  
  - `0x40021000` → RCC  
  - `0x40011000` → GPIOC  

### In GDB manually
```gdb
x/16wx 0x20000000    # View 16 words from SRAM start
x/16wx 0x40011000    # View GPIOC registers
```

---

## ✅ Quick Test

When flashed, the onboard **LED (PC13)** blinks every ~500ms.

---

## 📝 Notes

- If debugging fails with **firmware error**, update your ST-LINK firmware via STM32CubeProgrammer.  
- Always check jumper **JP5** (set to ST-LINK).  
- For mass erase or RDP reset, use:
  ```powershell
  STM32_Programmer_CLI -c port=SWD -e all
  ```

---

## 📌 Summary of Commands

### Build
```powershell
make clean
make
```

### Flash
```powershell
STM32_Programmer_CLI -c port=SWD -d build/main.bin 0x08000000 -rst
```

### Mass Erase
```powershell
STM32_Programmer_CLI -c port=SWD -e all
```

### Disable RDP
```powershell
STM32_Programmer_CLI -c port=SWD mode=UR -hardRst -ob RDP=0
```

### Debug (VS Code)
- Open project in VS Code
- Run **Cortex Debug** (F5)

---

## 🎯 Goal
This repo is a **bare-metal STM32 template** you can extend for:  
- GPIO, timers, USART, interrupts, etc.  
- Unit testing low-level drivers.  
- Educational projects without HAL/LL overhead.
