# TLSR8232 SDK Docker Environment

Docker-based build environment for developing custom firmware for the Telink TLSR8232 chipset.

The Docker image includes:

- Ubuntu 22.04
- TC32 GCC toolchain (Telink's custom compiler, not standard ARM)
- Telink TLSR8232 BLE SDK (`ble_sdk_hawk`)
- Build utilities: `make`, `wget`, `unzip`, `bzip2`

This avoids installing the SDK and compiler directly on your host machine and keeps builds reproducible.

---

## Build Image

```bash
docker build -t tlsr8232-sdk .
```

> **Apple Silicon**: the Dockerfile pins `--platform=linux/amd64` because the TC32
> toolchain is x86_64-only. A native arm64 image builds happily and then fails with
> `exec format error` the first time it runs `tc32-elf-gcc`.

---

## Build a Project

Run from the **project root directory**:

```bash
docker run --rm -v $(pwd):/src -w /src/examples/display -it tlsr8232-sdk make
```

Mount the **repo root**, not the example directory — examples that use `lib/`
(display, fonts, uart) reference it as `../../lib`.

> **Note**: `$SDK` is set in the image to `/opt/ble_sdk_hawk`, which is where the
> zip actually extracts to — despite being named `8232_BLE_SDK.zip`. No need to
> pass `SDK=` on the command line.

---

## Makefile Template

Copy this into your example's `Makefile` and change `TARGET`:

```makefile
TARGET=firmware
CC = $(TC32_HOME)/bin/tc32-elf-gcc
LD = $(TC32_HOME)/bin/tc32-elf-ld
CP = $(TC32_HOME)/bin/tc32-elf-objcopy
CCFLAGS = -Wall -std=gnu99 -DMCU_STARTUP_5316 -I $(SDK)/ -ffunction-sections -fdata-sections
LDFLAGS = --gc-sections -T $(SDK)/boot.link
LIB = $(SDK)/proj_lib/liblt_5316.a
BUILD_DIR = _build

DRIVERS_SRC = \
	$(SDK)/drivers/5316/gpio.c \
	$(SDK)/drivers/5316/analog.c \
	$(SDK)/drivers/5316/clock.c \
	$(SDK)/drivers/5316/bsp.c \
	$(SDK)/drivers/5316/spi.c \
	$(SDK)/drivers/5316/adc.c

DRIVERS_OBJS = $(addprefix $(BUILD_DIR)/drivers/, $(notdir $(DRIVERS_SRC:%.c=%.o)))
STARTUP_SRC = $(SDK)/boot/5316/cstartup_5316.S
STARTUP_OBJ = $(addprefix $(BUILD_DIR)/asm/, $(notdir $(STARTUP_SRC:%.S=%.o)))
SRC = main.c
OBJS = $(addprefix $(BUILD_DIR)/, $(SRC:%.c=%.o))

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET)
	$(CP) -O binary $< $@

$(BUILD_DIR)/$(TARGET): $(DRIVERS_OBJS) $(STARTUP_OBJ) $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(DRIVERS_OBJS) $(STARTUP_OBJ) $(LIB)

$(BUILD_DIR)/drivers/%.o: $(SDK)/drivers/5316/%.c
	mkdir -p $(BUILD_DIR)/drivers
	$(CC) -c $(CCFLAGS) -o $@ $<

$(BUILD_DIR)/asm/cstartup_5316.o: $(STARTUP_SRC)
	mkdir -p $(BUILD_DIR)/asm
	$(CC) -c $(CCFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(BUILD_DIR)
	$(CC) -c $(CCFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
```

---

## main.c Boilerplate

Every firmware needs this minimum structure:

```c
#include <stdint.h>
#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"

// Required by linker — do not remove!
_attribute_ram_code_ void irq_handler(void) {}

int main() {
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    // Your code here

    while (1);
    return 0;
}
```
