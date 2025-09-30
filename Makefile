# Project name
PROJECT = main

# Directories
BUILD_DIR   = build
SRC_DIR     = src
DRIVERS_DIR = $(SRC_DIR)/drivers
INC_DIR     = include
LINKER_DIR  = linker

# Toolchain
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
GDB     = arm-none-eabi-gdb
OPENOCD = "C:/Program Files/xpack-openocd-0.12.0-6/bin/openocd.exe"

# Flags
CFLAGS = -mcpu=cortex-m3 -mthumb -O0 -g3 -Wall -ffreestanding -fno-builtin \
         -DSTM32F103xB -I$(INC_DIR)
LDFLAGS = -T$(LINKER_DIR)/STM32F103RBTX_FLASH.ld -lc -lgcc -lnosys -Wl,--gc-sections

# Sources
C_SOURCES   = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(DRIVERS_DIR)/*.c)
ASM_SOURCES = $(wildcard $(SRC_DIR)/*.s)

# Objects
OBJECTS = $(C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) \
          $(ASM_SOURCES:$(SRC_DIR)/%.s=$(BUILD_DIR)/%.o)

# Default target
all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).bin

# Create build dir if not exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/drivers

# Compile C files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/drivers/%.o: $(DRIVERS_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)
	$(SIZE) $@


# Binary
$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

# Flash (requires STM32CubeProgrammer in PATH)
flash: $(BUILD_DIR)/$(PROJECT).bin
	STM32_Programmer_CLI -c port=SWD -d $< 0x08000000 -rst

# Debug with OpenOCD + GDB
debug: $(BUILD_DIR)/$(PROJECT).elf
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32f1x.cfg &
	sleep 1
	$(GDB) -ex "target remote localhost:3333" \
	       -ex "monitor reset halt" \
	       -ex "load" \
	       -ex "monitor reset init" \
	       -ex "continue" \
	       $(BUILD_DIR)/$(PROJECT).elf

# Chip erase (useful if RDP or corrupted flash)
erase:
	STM32_Programmer_CLI -c port=SWD -e all

# Clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean flash debug erase
