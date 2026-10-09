# Shared build rules for TLSR8232 examples. An example's Makefile sets TARGET and
# SRC, then includes this file:
#
#   TARGET = blink
#   SRC    = main.c
#   include ../../sdk/tlsr8232.mk
#
# Optional:
#   LIB_SRC  sources from lib/, e.g. display/display.c fonts/FreeMono9pt7b.c
#   SDK_SRC  SDK sources beyond the base drivers, e.g. drivers/5316/spi.c
#            common/string.c
#   DEFINES  extra -D flags
#
# Every build gets the base drivers (gpio, analog, clock, bsp, adc), the startup
# code, div_mod.S and libgcc; the linker drops whatever is not used.
# Output: _build/$(TARGET).bin — flash it at 0x0.

ROOT      := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
SDK       ?= $(ROOT)/sdk/tlsr8232
TC32_HOME ?= /opt/tc32
BUILD_DIR  = _build

CC   = $(TC32_HOME)/bin/tc32-elf-gcc
LD   = $(TC32_HOME)/bin/tc32-elf-ld
CP   = $(TC32_HOME)/bin/tc32-elf-objcopy
SIZE = $(TC32_HOME)/bin/tc32-elf-size

CCFLAGS = -Wall -std=gnu99 -DMCU_STARTUP_5316 $(DEFINES) -I $(SDK)/ -I $(ROOT)/lib \
	-ffunction-sections -fdata-sections -MMD -MP

LDFLAGS = --gc-sections -T $(SDK)/boot.link
LIBS    = $(SDK)/proj_lib/liblt_5316.a $(TC32_HOME)/lib/gcc/tc32-elf/4.5.1.tc32-elf-1.5/libgcc.a

SDK_BASE = $(addprefix drivers/5316/, gpio.c analog.c clock.c bsp.c adc.c)
SDK_ASM  = boot/5316/cstartup_5316.S div_mod.S

# Objects are namespaced by origin so same-named files cannot collide.
OBJS = \
	$(addprefix $(BUILD_DIR)/app/, $(SRC:.c=.o)) \
	$(addprefix $(BUILD_DIR)/lib/, $(LIB_SRC:.c=.o)) \
	$(addprefix $(BUILD_DIR)/sdk/, $(SDK_SRC:.c=.o) $(SDK_BASE:.c=.o) $(SDK_ASM:.S=.o))

all: $(BUILD_DIR)/$(TARGET).bin
	@$(SIZE) $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET)
	$(CP) -O binary $< $@

$(BUILD_DIR)/$(TARGET): $(OBJS) $(SDK)/boot.link
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(BUILD_DIR)/app/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CCFLAGS) -o $@ $<

$(BUILD_DIR)/lib/%.o: $(ROOT)/lib/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CCFLAGS) -o $@ $<

$(BUILD_DIR)/sdk/%.o: $(SDK)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CCFLAGS) -o $@ $<

$(BUILD_DIR)/sdk/%.o: $(SDK)/%.S
	@mkdir -p $(dir $@)
	$(CC) -c $(CCFLAGS) -o $@ $<

-include $(OBJS:.o=.d)

.PHONY: all clean
clean:
	rm -rf $(BUILD_DIR)
