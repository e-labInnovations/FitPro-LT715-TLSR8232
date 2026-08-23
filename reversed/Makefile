# Builds the decompiled stock firmware as C.
#
# `make` compiles src/firmware.c with the same tc32 toolchain the examples use,
# which is the point: it proves the decompilation is syntactically and typewise
# real C for this target, and it gives the compiler a chance to complain about
# places where the recovery is wrong.
#
# There is deliberately no target that produces a .bin. See README.md: the
# recompiled code cannot be the stock firmware, and a binary that looks
# flashable would be a trap.

TARGET = firmware

CC = $(TC32_HOME)/bin/tc32-elf-gcc
LD = $(TC32_HOME)/bin/tc32-elf-ld
CP = $(TC32_HOME)/bin/tc32-elf-objcopy

# -w by default: Ghidra's output produces thousands of legitimate warnings
# (implicit conversions between its recovered types). `make warn` shows them.
CCFLAGS = -std=gnu99 -DMCU_STARTUP_5316 -I include -I $(SDK)/ \
          -ffunction-sections -fdata-sections -fno-strict-aliasing -w

BUILD_DIR = _build

all: $(BUILD_DIR)/$(TARGET).o
	@echo
	@echo "compiled $(shell grep -c '^/\* ======== ' src/firmware.c) functions"
	@$(TC32_HOME)/bin/tc32-elf-size $(BUILD_DIR)/$(TARGET).o 2>/dev/null || true

$(BUILD_DIR)/$(TARGET).o: src/$(TARGET).c include/*.h
	mkdir -p $(BUILD_DIR)
	$(CC) -c $(CCFLAGS) -o $@ $<

# Same build with warnings on, into a log: the interesting ones are pointer/int
# mismatches, which mark places the decompiler guessed a type wrong.
warn:
	mkdir -p $(BUILD_DIR)
	$(CC) -c $(CCFLAGS:-w=) -Wall -o $(BUILD_DIR)/$(TARGET).o src/$(TARGET).c \
	  2> $(BUILD_DIR)/warnings.log; \
	  echo "$$(grep -c 'warning:' $(BUILD_DIR)/warnings.log) warnings -> $(BUILD_DIR)/warnings.log"

.PHONY: all warn clean
clean:
	rm -rf $(BUILD_DIR)
