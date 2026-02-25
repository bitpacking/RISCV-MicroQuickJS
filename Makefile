###### GD32V Makefile ######

######################################
# Target
######################################
TARGET = MicroQuickJS


######################################
# Source
######################################
# C sources
C_SOURCES =  \
$(wildcard GD32VF103_Firmware_Library_V1.6.0/Firmware/GD32VF103_standard_peripheral/Source/*.c) \
$(wildcard GD32VF103_Firmware_Library_V1.6.0/Firmware/GD32VF103_standard_peripheral/*.c) \
$(wildcard GD32VF103_Firmware_Library_V1.6.0/Firmware/RISCV/drivers/*.c) \
$(wildcard GD32VF103_Firmware_Library_V1.6.0/Firmware/RISCV/env_Eclipse/*.c) \
$(wildcard GD32VF103_Firmware_Library_V1.6.0/Firmware/RISCV/stubs/*.c) \

# add your c source here
C_SOURCES += \
$(wildcard ./*.c) \

# ASM sources
ASM_SOURCES =  \
GD32VF103_Firmware_Library_V1.6.0/Firmware/RISCV/env_Eclipse/start.s \
GD32VF103_Firmware_Library_V1.6.0/Firmware/RISCV/env_Eclipse/entry.s \


######################################
# Includes
######################################
# C includes
C_INCLUDES =  \
-I GD32VF103_Firmware_Library_V1.6.0/Firmware/GD32VF103_standard_peripheral/Include \
-I GD32VF103_Firmware_Library_V1.6.0/Firmware/GD32VF103_standard_peripheral \
-I GD32VF103_Firmware_Library_V1.6.0/Firmware/RISCV/drivers \
-I GD32VF103_Firmware_Library_V1.6.0/Firmware/RISCV/stubs \

# add your includes here
C_INCLUDES += \
-I . \

# AS includes
AS_INCLUDES = 


######################################
# Building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -Og

# Build path
BUILD_DIR = build


######################################
# Defines
######################################
# macros for gcc
# C defines
C_DEFS =  \
-D USE_STDPERIPH_DRIVER \
-D HXTAL_VALUE=8000000U \

# AS defines
AS_DEFS = 


######################################
# Firmware library
######################################
PERIFLIB_SOURCES = \
# $(wildcard Lib/*.a)


#######################################
# Linker
#######################################
# link script
LDSCRIPT = GD32VF103_Firmware_Library_V1.6.0/Firmware/RISCV/env_Eclipse/GD32VF103xB.lds


#######################################
# binaries
#######################################
PREFIX = riscv64-unknown-elf-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
AR = $(PREFIX)ar
SZ = $(PREFIX)size
OD = $(PREFIX)objdump
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 

#######################################
# CFLAGS
#######################################
# architecture
ARCH = -march=rv32imac -mabi=ilp32 -mcmodel=medlow

# compile gcc flags
ASFLAGS = $(ARCH) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wl,-Bstatic#, -ffreestanding -nostdlib

CFLAGS = $(ARCH) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wl,-Bstatic#, -ffreestanding -nostdlib

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" #-MT"$(@:%.o=%.d)"

# Generation a separate ELF section for each function and variable in the source file
# Cooperate -Wl,--gc-sections option to eliminating the unused code and data
# from the final executable
CFLAGS += -ffunction-sections -fdata-sections

# libraries
LIBS = -lc_nano -lm
LIBDIR = 
LDFLAGS = $(ARCH) -T$(LDSCRIPT) $(LIBDIR) $(LIBS) $(PERIFLIB_SOURCES) -Wl,--no-relax -Wl,--gc-sections -Wl,-Map,$(BUILD_DIR)/$(TARGET).map -nostartfiles #-ffreestanding -nostdlib

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


#######################################
# Build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "AS $<"
	@$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo "LD $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "OD $@"
	@$(OD) $(BUILD_DIR)/$(TARGET).elf -xS > $(BUILD_DIR)/$(TARGET).s $@
	@echo "SZ $@"
	@$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@

pp:
	$(CC) -E $(CFLAGS) main.c -o main.c.e

#######################################
# Clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)


#######################################
# Program
#######################################
flash: all
	@echo $@
	@$()openocd -f interface/cmsis-dap.cfg \
				-f target/gigadevice/gd32vf103.cfg \
				-c "adapter speed 5000" \
				-c "program $(BUILD_DIR)/$(TARGET).elf" \
				-c reset \
				-c shutdown

debug: all
	@echo $@
	@$()openocd -f interface/cmsis-dap.cfg \
				-f target/gigadevice/gd32vf103.cfg \
				-c "adapter speed 5000" \

dfu: all
	@echo $@
	@$()dfu-util -a 0 -s 0x08000000:leave -D $(BUILD_DIR)/$(TARGET).bin

#######################################
# dependencies
#######################################
# -include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***
