################################################################################
# 🧩 Project Configuration
################################################################################

# Project name
PROJECT = main

# Target selection: bootloader or application
TARGET ?= application

# Directories based on target
ifeq ($(TARGET), bootloader)
TARGET_DEFINES = -DBOOTLOADER
FLASH_START_ADDRESS = 0x08000000
TARGET_SRC_DIR     = bootloader/src
TARGET_INC_DIR     = bootloader/include
TARGET_LINKER_FILE = bootloader/linker/STM32F103RBTX_BOOT.ld
BUILD_DIR          = build/bootloader
else
TARGET_DEFINES = -DAPPLICATION
FLASH_START_ADDRESS = 0x08004000
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
         $(TARGET_DEFINES)

LDFLAGS = -T$(TARGET_LINKER_FILE) -lc -lgcc -Wl,--gc-sections

################################################################################
# 📂 Source and Object Files
################################################################################

# Recursive wildcard function
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

# Collect sources
C_SOURCES := $(call rwildcard,$(TARGET_SRC_DIR),*.c) \
             $(call rwildcard,$(COMMON_SRC_DIR),*.c)

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

all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).bin

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
# 🧹 Cleaning
################################################################################

clean:
ifeq ($(OS),Windows_NT)
	@if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
else
	@rm -rf $(BUILD_DIR)
endif
	@echo [CLEAN] Removed build directory.

################################################################################
# 📘 Phony Targets
################################################################################

.PHONY: all clean flash debug erase
