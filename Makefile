################################################################################
# 🧩 Project Configuration
################################################################################

# Target selection: bootloader or application
TARGET ?= application

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

################################################################################
# 🧠 Toolchain
################################################################################

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
GDB     = arm-none-eabi-gdb
OPENOCD = "C:/Program Files/xpack-openocd-0.12.0-6/bin/openocd.exe"

################################################################################
# ⚙️ Compiler and Linker Flags
################################################################################

CFLAGS = -mcpu=cortex-m3 -mthumb -O0 -g3 -Wall -ffreestanding -fno-builtin \
         -DSTM32F103xB -I$(TARGET_INC_DIR) -I$(COMMON_INC_DIR) \
         -Icommon/crypto/micro-ecc/include -Icommon/crypto/micro-ecc/asm -Ikeys \
         $(TARGET_DEFINES)

LDFLAGS = -T$(TARGET_LINKER_FILE) -lc -lgcc -Wl,--gc-sections

################################################################################
# 📂 Source and Object Files
################################################################################

# Recursive wildcard function
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

# Collect sources
C_SOURCES := $(call rwildcard,$(TARGET_SRC_DIR),*.c) \
             $(call rwildcard,$(COMMON_SRC_DIR),*.c) \
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
	@echo [SIGN] Signing firmware with ECDSA...
	@python scripts/sign_firmware.py $@
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
# 📘 Phony Targets
################################################################################

.PHONY: all clean flash debug erase bl app both flash-bl flash-app flash-both size genkeys sign

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
