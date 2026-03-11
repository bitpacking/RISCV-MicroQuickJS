###### GD32V Makefile ######

######################################
# Target
######################################
TARGET = MicroQuickJS


######################################
# Host tool 
######################################
HOST_TARGET = mqjs_stdlib
EXE = .exe

HOST_CC = gcc
HOST_CFLAGS = -I mquickjs -Wall

# header file name of js stdlib
STDLIB_HEADER = js_stdlib.h
GENERATED_HEADERS = mquickjs_atom.h $(STDLIB_HEADER)

HOST_SOURCE = \
user_stdlib.c \
mquickjs/mquickjs_build.c \

# exclude host c source file
EXCLUDE_SOURCES = \
./user_stdlib.c \
./mqjs_stdlib.c \

# js script file
JS_SCRIPT = script.js

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

# for MicroQuickJS
C_SOURCES += \
mquickjs/dtoa.c \
mquickjs/libm.c \
mquickjs/cutils.c \
mquickjs/mquickjs.c

# for MicroQuickJS REPL
C_SOURCES += mquickjs/readline.c

# add all c file in the root dir
C_SOURCES += \
$(filter-out $(EXCLUDE_SOURCES), $(wildcard ./*.c)) \

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

# for MicroQuickJS
C_INCLUDES += \
-I mquickjs \

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
OPT = -Os

# Build path
BUILD_DIR = build
HOST_BUILD_DIR = $(BUILD_DIR)/host


######################################
# Defines
######################################
# macros for gcc
# C defines
C_DEFS =  \
-D USE_STDPERIPH_DRIVER \
-D HXTAL_VALUE=8000000U 

# for mquickjs
C_DEFS += \
-D USE_SOFTFLOAT

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
ARCH = -march=rv32imac_zicsr -mabi=ilp32 -mcmodel=medlow

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

# Build host tool
HOST_OBJECTS = $(addprefix $(HOST_BUILD_DIR)/, $(notdir $(HOST_SOURCE:.c=.o)))

$(HOST_BUILD_DIR)/%.o: %.c Makefile | $(HOST_BUILD_DIR)
	@echo "HOST CC $<"
	@$(HOST_CC) -c $(HOST_CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" $< -o $@

$(HOST_TARGET)$(EXE): $(HOST_OBJECTS) 
	@echo "HOST LD $@"
	@$(HOST_CC) -o $@ $^

mquickjs_atom.h: $(HOST_TARGET)$(EXE)
	@echo "GE $@"
	@./$(HOST_TARGET)$(EXE) -a -m32 > $@

$(STDLIB_HEADER): $(HOST_TARGET)$(EXE)
	@echo "GE $@"
	@./$(HOST_TARGET)$(EXE) -m32 > $@

$(HOST_BUILD_DIR):
	mkdir $@

# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# for js script file
OBJECTS += $(BUILD_DIR)/$(JS_SCRIPT:.js=.o)
$(BUILD_DIR)/%.o: %.js Makefile | $(BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(ARCH) -nostdlib -r -Wl,-b,binary $< -o $@

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) $(GENERATED_HEADERS)
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
	@echo "CP $@"
	@$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "CP $@"
	@$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@

#######################################
# Clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)
	-rm $(HOST_TARGET)$(EXE)
	-rm $(GENERATED_HEADERS)


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
				-c "adapter speed 5000"

dfu: all
	@echo $@
	@$()dfu-util -a 0 -s 0x08000000:leave -D $(BUILD_DIR)/$(TARGET).bin

#######################################
# dependencies
#######################################
# -include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)
-include $(wildcard $(BUILD_DIR)/*.d)
-include $(wildcard $(HOST_BUILD_DIR)/*.d)

# *** EOF ***
