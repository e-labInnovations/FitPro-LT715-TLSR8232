# TLSR8232 Build Environment

Everything needed to build firmware for the FitPro LT715 (Telink TLSR8232):

| Path           | What                                                         |
| -------------- | ------------------------------------------------------------ |
| `tlsr8232/`    | Telink BLE SDK `telink_hawk_ble_sdk_v1.3.1`, as shipped       |
| `tlsr8232.mk`  | Shared build rules every example includes                    |
| `Dockerfile`   | Build image: TC32 GCC and `make`, nothing else               |

The SDK lives in the repo, not in the image, so builds don't depend on Telink's
download links staying up, and an SDK patch is an ordinary commit that takes
effect without rebuilding the image.

---

## The Image

```
ghcr.io/e-labinnovations/tlsr8232-sdk
```

- Debian bookworm-slim
- TC32 GCC 4.5.1 (`tc32_gcc_v2.0`) at `/opt/tc32` (`$TC32_HOME`, on `PATH`):
  compiler, assembler and binutils. The debuggers (openocd, gdb, tcdb), docs
  and test binaries are removed.
- `make`

About 37 MB to download (150 MB unpacked). It is `linux/amd64` only, because
the TC32 toolchain only exists as x86_64 Linux binaries. Docker Desktop on
Apple Silicon runs it under emulation, and prints a platform warning.

### Building it locally

```bash
docker build --platform linux/amd64 -t ghcr.io/e-labinnovations/tlsr8232-sdk sdk/
```

`--platform` matters on Apple Silicon: without it Docker tags the image as
arm64 and `docker run` won't find it.

The Dockerfile downloads the toolchain from Telink's mirror
(`TC32_URL`) and checks its SHA-256 (`TC32_SHA256`), so a changed or
truncated download fails the build. If the mirror disappears, pass
`--build-arg TC32_URL=<another copy>`; the published image also keeps a
working copy of the toolchain.

### Publishing

[.github/workflows/sdk-image.yml](../.github/workflows/sdk-image.yml) builds
the image when `sdk/Dockerfile` changes on `main` and pushes it to GHCR, tagged
`latest` and with the commit's short SHA. It then builds every example in the
new image. It can also be run by hand from the Actions tab.

---

## Build an Example

Run from the **repo root**:

```bash
docker run --rm -v "$(pwd)":/src -w /src/examples/display ghcr.io/e-labinnovations/tlsr8232-sdk make
```

Windows (PowerShell): `-v "${PWD}:/src"`; cmd.exe: `-v "%cd%:/src"`.

Mount the repo root, not the example directory: examples use `lib/` and `sdk/`
through `../../`. The output is `examples/<name>/_build/<name>.bin`.

---

## Writing an Example

```
examples/blink/
├── Makefile
└── main.c
```

`Makefile`:

```makefile
TARGET  = blink
SRC     = main.c
include ../../sdk/tlsr8232.mk
```

| Variable  | Meaning                                                            |
| --------- | ------------------------------------------------------------------ |
| `TARGET`  | Output name                                                        |
| `SRC`     | The example's own sources                                          |
| `LIB_SRC` | Sources from `lib/`, e.g. `display/display.c fonts/FreeMono9pt7b.c` |
| `SDK_SRC` | SDK sources beyond the base drivers, e.g. `drivers/5316/spi.c common/string.c` |
| `DEFINES` | Extra `-D` flags                                                   |

Every build already gets the base drivers (`gpio`, `analog`, `clock`, `bsp`,
`adc`), the startup code, `div_mod.S` (integer division) and libgcc. The linker
drops whatever the program doesn't use. `lib/` is on the include path, so
`#include "display/display.h"` works.

`main.c` needs at least:

```c
#include <stdint.h>
#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"

// Required by the linker: do not remove.
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

---

## SDK Contents

`tlsr8232/` is `ble_sdk_hawk/` from Telink's `8232_BLE_SDK.zip`
(SHA-256 `7d9bbe4f…6e183425`), unchanged apart from dropping
`tl_check_fw2.exe`, a Windows-only tool. The zip's release note is kept
alongside. The SDK files are © Telink Semiconductor; see their headers for
terms.
