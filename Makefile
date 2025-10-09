################################################################################
# 🧩 Project Configuration
################################################################################

# Project name
PROJECT = main

# Directory structure
BUILD_DIR   = build
SRC_DIR     = src
DRIVERS_DIR = $(SRC_DIR)/drivers
INC_DIR     = include
LINKER_DIR  = linker

################################################################################
# 🧠 Toolchain and Utilities
################################################################################

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
GDB     = arm-none-eabi-gdb
OPENOCD = "C:/Program Files/xpack-openocd-0.12.0-6/bin/openocd.exe"

################################################################################
# ⚙️ Compiler and Linker Flags
################################################################################

# Compiler options
CFLAGS = -mcpu=cortex-m3 -mthumb -O0 -g3 -Wall -ffreestanding -fno-builtin \
         -DSTM32F103xB -I$(INC_DIR)

# Linker options
LDFLAGS = -T$(LINKER_DIR)/STM32F103RBTX_FLASH.ld \
           -lc -lgcc -lnosys -Wl,--gc-sections

################################################################################
# 📂 Source and Object Management
################################################################################

# Find all C and Assembly source files
C_SOURCES   = $(wildcard $(SRC_DIR)/*.c) \
              $(wildcard $(DRIVERS_DIR)/*.c)

ASM_SOURCES = $(wildcard $(SRC_DIR)/*.s)

# Object file list (mirror directory structure under build/)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) \
          $(patsubst $(SRC_DIR)/%.s, $(BUILD_DIR)/%.o, $(ASM_SOURCES))

# Extract unique subdirectories needed in build/
BUILD_SUBDIRS := $(sort $(dir $(OBJECTS)))

################################################################################
# 🎯 Default Target
################################################################################

all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).bin

################################################################################
# 🧱 Directory Creation
################################################################################

# Cross-platform directory creation
$(BUILD_SUBDIRS):
ifeq ($(OS),Windows_NT)
	@if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
else
	@mkdir -p $@
endif

################################################################################
# 🔧 Compilation Rules
################################################################################

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_SUBDIRS)
	@echo [CC] $<
	@$(CC) $(CFLAGS) -c $< -o $@

# Assemble ASM source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s | $(BUILD_SUBDIRS)
	@echo [AS] $<
	@$(CC) $(CFLAGS) -c $< -o $@

################################################################################
# 🔗 Linking and Binary Generation
################################################################################

# Link all object files
$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS)
	@echo [LD] $@
	@$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)
	@$(SIZE) $@

# Generate binary from ELF
$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	@echo [BIN] $@
	@$(OBJCOPY) -O binary $< $@

################################################################################
# 🚀 Flashing and Debugging
################################################################################

# Flash binary using STM32CubeProgrammer
flash: $(BUILD_DIR)/$(PROJECT).bin
	@echo [FLASH] Programming MCU...
	@STM32_Programmer_CLI -c port=SWD -d $< 0x08000000 -rst

# Debug session with OpenOCD + GDB
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

# Chip erase (full)
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
