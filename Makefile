################################################################################
# 🧩 Project Configuration
################################################################################

# Target selection: bootloader or application
TARGET ?= application

# Footer type (default: sec)
# Set via: make app sec | make app crc | make app none
FOOTER ?= sec

# Directories based on target
ifeq ($(TARGET), bootloader)
    PROJECT = bootloader
    TARGET_DEFINES = -DBOOTLOADER -DBOOTLOADER_START=0x08000000
    FLASH_START_ADDRESS = 0x08000000
    TARGET_SRC_DIR     = bootloader/src
    TARGET_INC_DIR     = bootloader/include
    TARGET_LINKER_FILE = bootloader/linker/STM32F103RBTX_BOOT.ld
    BUILD_DIR          = build/bootloader
else
    PROJECT = app
    TARGET_DEFINES = -DAPPLICATION -DAPPLICATION_START=0x08008000
    FLASH_START_ADDRESS = 0x08008000
    TARGET_SRC_DIR     = application/src
    TARGET_INC_DIR     = application/include
    TARGET_LINKER_FILE = application/linker/STM32F103RBTX_APP.ld
    BUILD_DIR          = build/application
endif

# Common code
COMMON_SRC_DIR = common/src
COMMON_INC_DIR = common/include
COMMON_UART_DIR = common/uart

################################################################################
# 🧠 Toolchain
################################################################################

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
GDB     = arm-none-eabi-gdb
OPENOCD = "C:/Program Files/xpack-openocd-0.12.0-6/bin/openocd.exe"
CPPCHECK = "C:/Program Files/Cppcheck/cppcheck.exe"

################################################################################
# ⚙️ Compiler and Linker Flags
################################################################################

CFLAGS = -mcpu=cortex-m3 -mthumb -O0 -g3 -Wall -ffreestanding -fno-builtin \
         -DSTM32F103xB -I$(TARGET_INC_DIR) -I$(COMMON_INC_DIR) -I$(COMMON_UART_DIR) \
         -Icommon/crypto/micro-ecc/include -Icommon/crypto/micro-ecc/asm -Ikeys \
         $(TARGET_DEFINES)

LDFLAGS = -T$(TARGET_LINKER_FILE) -lc -lgcc -Wl,--gc-sections

# Generate .map file
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map

################################################################################
# 📂 Source and Object Files
################################################################################

# Recursive wildcard function
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

# Collect sources
C_SOURCES := $(call rwildcard,$(TARGET_SRC_DIR),*.c) \
             $(call rwildcard,$(COMMON_SRC_DIR),*.c) \
             $(call rwildcard,$(COMMON_UART_DIR),*.c) \
             common/crypto/micro-ecc/src/uECC.c

# Explicitly include startup file
STARTUP_FILE := $(TARGET_SRC_DIR)/startup_stm32f103rbtx.s

# Other ASM sources
ASM_SOURCES := $(call rwildcard,$(TARGET_SRC_DIR),*.s)

# Map source paths to object paths under build/
OBJECTS := $(patsubst $(TARGET_SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS := $(patsubst $(COMMON_SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(OBJECTS))

# Add startup object explicitly
OBJECTS += $(BUILD_DIR)/startup_stm32f103xb.o

# Build subdirectories
BUILD_SUBDIRS := $(sort $(dir $(OBJECTS)))

################################################################################
# 🎯 Default Target
################################################################################

all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).bin $(BUILD_DIR)/$(PROJECT).hex

################################################################################
# 🧱 Create Build Directories
################################################################################

$(BUILD_SUBDIRS):
ifeq ($(OS),Windows_NT)
	@if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
else
	@mkdir -p $@
endif

################################################################################
# 🔧 Compilation Rules
################################################################################

# Compile target C files
$(BUILD_DIR)/%.o: $(TARGET_SRC_DIR)/%.c | $(BUILD_SUBDIRS)
	@echo [CC] $<
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile common C files
$(BUILD_DIR)/%.o: $(COMMON_SRC_DIR)/%.c | $(BUILD_SUBDIRS)
	@echo [CC] $<
	@$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup ASM file explicitly
$(BUILD_DIR)/startup_stm32f103xb.o: $(STARTUP_FILE) | $(BUILD_SUBDIRS)
	@echo [AS] $<
	@$(CC) $(CFLAGS) -c $< -o $@

# Assemble other ASM files
$(BUILD_DIR)/%.o: $(TARGET_SRC_DIR)/%.s | $(BUILD_SUBDIRS)
	@echo [AS] $<
	@$(CC) $(CFLAGS) -c $< -o $@

################################################################################
# 🔗 Linking and Binary Generation
################################################################################

$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS)
	@echo [LD] $@
	@$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)
	@$(SIZE) $@

$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	@echo [BIN] $@
	@$(OBJCOPY) -O binary $< $@
ifeq ($(TARGET), application)
ifeq ($(FOOTER), sec)
	@echo [SIGN] Signing firmware with ECDSA...
	@python scripts/sign_firmware.py $@
else ifeq ($(FOOTER), crc)
	@echo [CRC] Adding CRC footer...
	@python scripts/add_crc_footer.py $@
else ifeq ($(FOOTER), none)
	@echo [INFO] No footer added (raw binary)
else
	@echo [SIGN] Signing firmware with ECDSA (default)...
	@python scripts/sign_firmware.py $@
endif
endif

$(BUILD_DIR)/$(PROJECT).hex: $(BUILD_DIR)/$(PROJECT).elf
	@echo [HEX] $@
	@$(OBJCOPY) -O ihex $< $@

################################################################################
# 🚀 Flashing and Debugging
################################################################################

flash: $(BUILD_DIR)/$(PROJECT).bin
	@echo [FLASH] Programming MCU...
	@STM32_Programmer_CLI -c port=SWD -d $< $(FLASH_START_ADDRESS) -rst

debug: $(BUILD_DIR)/$(PROJECT).elf
	@echo [DEBUG] Starting OpenOCD + GDB...
	@$(OPENOCD) -f interface/stlink.cfg -f target/stm32f1x.cfg &
	@sleep 1
	@$(GDB) -ex "target remote localhost:3333" \
	        -ex "monitor reset halt" \
	        -ex "load" \
	        -ex "monitor reset init" \
	        -ex "continue" \
	        $(BUILD_DIR)/$(PROJECT).elf

erase:
	@echo [ERASE] Erasing MCU flash...
	@STM32_Programmer_CLI -c port=SWD -e all

################################################################################
# 🔥 Convenient Shortcuts
################################################################################

.PHONY: bl app both flash-bl flash-app size

# Short aliases for building
bl:
	@$(MAKE) TARGET=bootloader

app:
	@$(MAKE) TARGET=application

both: bl app

# Flash shortcuts
flash-bl: bl
	@echo [FLASH] Programming Bootloader...
	@STM32_Programmer_CLI -c port=SWD -d build/bootloader/bootloader.bin 0x08000000 -rst

flash-app: app
	@echo [FLASH] Programming Application...
	@STM32_Programmer_CLI -c port=SWD -d build/application/app.bin 0x08008000 -rst

flash-both: both
	@echo [FLASH] Programming Bootloader + Application...
	@STM32_Programmer_CLI -c port=SWD -d build/bootloader/bootloader.bin 0x08000000
	@STM32_Programmer_CLI -c port=SWD -d build/application/app.bin 0x08008000 -rst

# Size report
size:
	@echo "==================================="
	@echo "         Size Report"
	@echo "==================================="
	@if exist "build\\bootloader\\bootloader.elf" (\
		echo Bootloader: & $(SIZE) build/bootloader/bootloader.elf\
	)
	@if exist "build\\application\\app.elf" (\
		echo Application: & $(SIZE) build/application/app.elf\
	)
	@echo "==================================="

# Run Host
runhost:
	@python scripts/host_script.py COM4

################################################################################
# 🧹 Cleaning
################################################################################

clean:
ifeq ($(OS),Windows_NT)
	@if exist "build" rmdir /S /Q "build"
else
	@rm -rf build
endif
	@echo [CLEAN] Removed entire build directory.

################################################################################
# 📘 Help Target
################################################################################

.PHONY: help
help:
	@echo ========================================================================
	@echo   STM32F103RB Baremetal Project - Available Targets
	@echo ========================================================================
	@echo.
	@echo BUILD TARGETS:
	@echo   make bl              - Build bootloader only
	@echo   make app             - Build application only (default: signed)
	@echo   make both            - Build both bootloader and application
	@echo   make all             - Same as 'make app'
	@echo.
	@echo FLASH TARGETS:
	@echo   make flash           - Flash current target (bootloader or app)
	@echo   make flash-bl        - Build and flash bootloader
	@echo   make flash-app       - Build and flash application
	@echo   make flash-both      - Build and flash both
	@echo.
	@echo APPLICATION FOOTER OPTIONS:
	@echo   make sec             - Build app with ECDSA signature (default, secure)
	@echo   make crc             - Build app with CRC32 footer (legacy)
	@echo   make none            - Build app without footer (raw binary)
	@echo.
	@echo UTILITY TARGETS:
	@echo   make clean           - Remove all build artifacts
	@echo   make size            - Display memory usage for both targets
	@echo   make erase           - Erase entire MCU flash memory
	@echo   make debug           - Start OpenOCD + GDB debugging session
	@echo   make runhost         - Run host script for UART bootloader
	@echo   make genkeys         - Generate ECDSA secp256r1 key pair
	@echo   make sign            - Manually sign application firmware
	@echo   make analyze         - Run static analysis on entire project
	@echo   make help            - Display this help message
	@echo.
	@echo ADVANCED OPTIONS:
	@echo   TARGET=bootloader    - Explicitly set target (default: application)
	@echo   FOOTER=sec^|crc^|none  - Set footer type for application build
	@echo.
	@echo EXAMPLES:
	@echo   make clean ^&^& make both        - Clean build of everything
	@echo   make app FOOTER=crc           - Build app with CRC footer
	@echo   make flash-both               - Flash complete system
	@echo   make size                     - Check memory usage
	@echo.
	@echo For more information, see README.md
	@echo ========================================================================

################################################################################
# 📘 Phony Targets
################################################################################

.PHONY: all clean flash debug erase bl app both flash-bl flash-app flash-both size genkeys sign sec crc none help analyze analyze-bootloader analyze-app analyze-report

# Footer type targets - set FOOTER and build application
sec:
	@$(MAKE) app FOOTER=sec

crc:
	@$(MAKE) app FOOTER=crc

none:
	@$(MAKE) app FOOTER=none

################################################################################
# 🔑 Secure Boot: Key Generation and Signing
################################################################################

# Generate ECDSA key pair for secure boot
genkeys:
	@echo [KEYS] Generating ECDSA secp256r1 key pair...
	@python scripts/generate_keys.py

# Sign application firmware (manual trigger if not auto-signed during build)
sign:
	@echo [SIGN] Signing application firmware...
	@python scripts/sign_firmware.py build/application/app.bin

################################################################################
# 🔍 Static Analysis
################################################################################

.PHONY: analyze analyze-bootloader analyze-app

# Analyze entire project
analyze:
	@echo [ANALYZE] Running static analysis on entire project...
	@$(CPPCHECK) --enable=all \
	          --suppress=missingIncludeSystem \
	          --suppress=unusedFunction \
	          --suppressions-list=suppressions.txt \
	          --std=c11 \
	          --platform=unix32 \
	          --inline-suppr \
	          --quiet \
	          --force \
	          -I$(COMMON_INC_DIR) \
	          -I$(COMMON_UART_DIR) \
	          -Icommon/crypto/micro-ecc/include \
	          -Ibootloader/include \
	          -Iapplication/include \
	          bootloader/src common/src common/uart application/src
	@echo [ANALYZE] Analysis complete!

# Analyze bootloader only
analyze-bootloader:
	@echo [ANALYZE] Analyzing bootloader...
	@$(CPPCHECK) --enable=all \
	          --suppress=missingIncludeSystem \
	          --suppressions-list=suppressions.txt \
	          --std=c11 \
	          -I$(COMMON_INC_DIR) \
	          -I$(COMMON_UART_DIR) \
	          -Icommon/crypto/micro-ecc/include \
	          -Ibootloader/include \
	          bootloader/src
	@echo [ANALYZE] Bootloader analysis complete!

# Analyze application only
analyze-app:
	@echo [ANALYZE] Analyzing application...
	@$(CPPCHECK) --enable=all \
	          --suppress=missingIncludeSystem \
	          --suppressions-list=suppressions.txt \
	          --std=c11 \
	          -I$(COMMON_INC_DIR) \
	          -I$(COMMON_UART_DIR) \
	          -Iapplication/include \
	          application/src
	@echo [ANALYZE] Application analysis complete!

