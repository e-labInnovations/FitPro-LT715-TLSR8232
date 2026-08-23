TARGET = battery_charge

CC  = $(TC32_HOME)/bin/tc32-elf-gcc
LD  = $(TC32_HOME)/bin/tc32-elf-ld
CP  = $(TC32_HOME)/bin/tc32-elf-objcopy

CCFLAGS = -Wall -std=gnu99 -DMCU_STARTUP_5316 \
          -I $(SDK)/ -I ../../lib \
          -ffunction-sections -fdata-sections -MMD -MP

LDFLAGS = --gc-sections -T $(SDK)/boot.link

LIB     = $(SDK)/proj_lib/liblt_5316.a
LIBGCC  = $(TC32_HOME)/lib/gcc/tc32-elf/4.5.1.tc32-elf-1.5/libgcc.a

BUILD_DIR = _build

DRIVERS_SRC = \
	$(SDK)/drivers/5316/gpio.c \
	$(SDK)/drivers/5316/analog.c \
	$(SDK)/drivers/5316/clock.c \
	$(SDK)/drivers/5316/bsp.c \
	$(SDK)/drivers/5316/spi.c \
	$(SDK)/drivers/5316/uart.c \
	$(SDK)/drivers/5316/adc.c
DRIVERS_OBJS = $(addprefix $(BUILD_DIR)/drivers/, $(notdir $(DRIVERS_SRC:%.c=%.o)))

SDK_COMMON_SRC = $(SDK)/common/string.c
SDK_COMMON_OBJS = $(addprefix $(BUILD_DIR)/sdk-common/, $(notdir $(SDK_COMMON_SRC:%.c=%.o)))

LIB_SRC = \
	../../lib/display/display.c \
	../../lib/display/gfx.c \
	../../lib/fonts/FreeMono9pt7b.c \
	../../lib/battery/battery.c
LIB_OBJS = $(addprefix $(BUILD_DIR)/lib/, $(notdir $(LIB_SRC:%.c=%.o)))

STARTUP_SRC = $(SDK)/boot/5316/cstartup_5316.S
STARTUP_OBJ = $(BUILD_DIR)/asm/cstartup_5316.o

DIVMOD_SRC = $(SDK)/div_mod.S
DIVMOD_OBJ = $(BUILD_DIR)/asm/div_mod.o

SRC  = main.c
OBJS = $(addprefix $(BUILD_DIR)/, $(SRC:%.c=%.o))

all: $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET)
	$(CP) -O binary $< $@

$(BUILD_DIR)/$(TARGET): $(OBJS) $(LIB_OBJS) $(SDK_COMMON_OBJS) $(DRIVERS_OBJS) $(STARTUP_OBJ) $(DIVMOD_OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIB_OBJS) $(SDK_COMMON_OBJS) $(DRIVERS_OBJS) $(STARTUP_OBJ) $(DIVMOD_OBJ) $(LIB) $(LIBGCC)

# Drivers
$(BUILD_DIR)/drivers/%.o: $(SDK)/drivers/5316/%.c
	mkdir -p $(BUILD_DIR)/drivers
	$(CC) -c $(CCFLAGS) -o $@ $<

# SDK common (string)
$(BUILD_DIR)/sdk-common/%.o: $(SDK)/common/%.c
	mkdir -p $(BUILD_DIR)/sdk-common
	$(CC) -c $(CCFLAGS) -o $@ $<

# Local libs — first matching prerequisite wins
$(BUILD_DIR)/lib/%.o: ../../lib/display/%.c
	mkdir -p $(BUILD_DIR)/lib
	$(CC) -c $(CCFLAGS) -o $@ $<

$(BUILD_DIR)/lib/%.o: ../../lib/fonts/%.c
	mkdir -p $(BUILD_DIR)/lib
	$(CC) -c $(CCFLAGS) -o $@ $<

$(BUILD_DIR)/lib/%.o: ../../lib/battery/%.c
	mkdir -p $(BUILD_DIR)/lib
	$(CC) -c $(CCFLAGS) -o $@ $<

# Startup
$(BUILD_DIR)/asm/cstartup_5316.o: $(STARTUP_SRC)
	mkdir -p $(BUILD_DIR)/asm
	$(CC) -c $(CCFLAGS) $< -o $@

# div_mod
$(BUILD_DIR)/asm/div_mod.o: $(DIVMOD_SRC)
	mkdir -p $(BUILD_DIR)/asm
	$(CC) -c $(CCFLAGS) $< -o $@

# App sources
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(BUILD_DIR)
	$(CC) -c $(CCFLAGS) -o $@ $<

# Rebuild objects when the headers they include change
-include $(shell find $(BUILD_DIR) -name "*.d" 2>/dev/null)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
