# FitPro LT715/LT716 — Custom Firmware

Reverse engineering and custom firmware for the **FitPro LT715/LT716**
smartwatch, built on the **Telink TLSR8232** SoC.

[![Hacking a ₹249 Smartwatch: Custom Firmware on TLSR8232](https://elabins.com/widgets/post/hacking-a-249-smartwatch-custom-firmware-on-tlsr8232.svg)](https://elabins.com/blog/hacking-a-249-smartwatch-custom-firmware-on-tlsr8232)

<!-- Photo of the watch: add docs/images/lt715.jpg, then uncomment.
![FitPro LT715](docs/images/lt715.jpg)
-->

## Hardware

| Part    | Details                                                   |
| ------- | --------------------------------------------------------- |
| SoC     | Telink TLSR8232F512/F128 ET24 (24-pin, no RST pin), BLE   |
| Display | ST7735 128×128 TFT (module XHZY14413-02)                  |
| Input   | Touch key on PC2                                          |
| Motor   | Vibrator on PA5                                           |
| Power   | LiPo; battery and charger sensing on the ADC (PB2)        |

Full pin map and board notes: [docs/hardware.md](docs/hardware.md).

## Status

- ✅ Display, graphics, fonts and images
- ✅ Touch key (tap, double tap, long press)
- ✅ Vibrator
- ✅ Battery gauge and charger detection, matching the stock firmware
- ✅ UART debug output
- ✅ BLE advertising
- ✅ Stock firmware decompiled to C and all its assets extracted

## Quick Start

You need Docker and, for flashing, a USB-to-SWire bridge (see
[docs/flashing.md](docs/flashing.md)).

Build an example (from the repo root):

```bash
docker run --rm -v "$(pwd)":/src -w /src/examples/display ghcr.io/e-labinnovations/tlsr8232-sdk make
```

Flash it:

```bash
python tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 write_flash examples/display/_build/display.bin
```

Writing your own example: [sdk/README.md](sdk/README.md).

## Repository

| Path              | What                                                       |
| ----------------- | ---------------------------------------------------------- |
| `examples/`       | Small firmware projects, one per feature                   |
| `lib/`            | Drivers: display, fonts, touch, vibrate, battery, uart     |
| `sdk/`            | Build image, shared make rules, Telink SDK                 |
| `tools/`          | Firmware explorer, asset dumper, image converter, SWire flasher |
| `ghidra_project/` | Ghidra database and scripts for the stock firmware         |
| `reversed/`       | The stock firmware decompiled as a buildable C project     |
| `assets/`         | Fonts, strings and graphics extracted from the stock firmware |
| `binaries/`       | Stock firmware dump, ATC OEPL firmware, SWire bridge firmware |

## Documentation

| Doc                                              | About                                              |
| ------------------------------------------------ | -------------------------------------------------- |
| [Hardware](docs/hardware.md)                     | Pins, display, battery, touch, motor, test pads    |
| [Flashing](docs/flashing.md)                     | SWire recovery with a Blue Pill, OTA over BLE      |
| [Finding the pins](docs/pin-hunting.md)          | Probe firmware, sweeps, reading pins out of the firmware |
| [Stock firmware](docs/stock-firmware.md)         | Layout, decompilation, lifecycle, the dumps        |
| [Stock assets](docs/assets.md)                   | Fonts, strings and graphics dumped to PNG and C    |
| [Battery and charging](docs/battery.md)          | The stock gauge and charger detection, decoded     |
| [SDK and build](sdk/README.md)                   | Build image, make rules, writing an example        |

## References

| Resource                       | Link                                                                      |
| ------------------------------ | ------------------------------------------------------------------------- |
| rbaron/m6-reveng               | https://github.com/rbaron/m6-reveng                                       |
| pvvx/TlsrTools                 | https://github.com/pvvx/TlsrTools                                         |
| pvvx/TLSRPGM                   | https://github.com/pvvx/TLSRPGM                                           |
| OpenEPaperLink                 | https://github.com/OpenEPaperLink/OpenEPaperLink                          |
| amir1387aht/phy6222_smartwatch | https://github.com/amir1387aht/phy6222_smartwatch                         |
| atc1441 BLE OTA Flasher        | https://atc1441.github.io/BLE_EPaper_OTA.html                             |
| Telink 8232 BLE SDK            | http://wiki.telink-semi.cn/tools_and_sdk/BLE/8232_BLE_SDK.zip             |
| TC32 GCC Toolchain             | http://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tc32_gcc_v2.0.tar.bz2 |
