# Hacking FitPro LT715/LT716 Watch

Reverse engineering and custom firmware development for the **FitPro LT715/LT716 Smartwatch**, which uses the **Telink TLSR8232** SoC.

The goal is to replace the stock firmware with a custom one for my new project.

---

## Hardware

| Component      | Details                                            |
| -------------- | -------------------------------------------------- |
| Watch model    | FitPro LT715/LT716                                 |
| MCU            | Telink TLSR8232F512/F128 ET24 (24-pin, no RST pin) |
| SOC ID         | `0x5316`                                           |
| Display        | ST7735 128x128 TFT                                 |
| Display module | XHZY14413-02                                       |
| BLE            | Built-in (TLSR8232)                                |

### Display Pin Mapping

| Signal | GPIO | MCU Pin | FPC Pin |
| ------ | ---- | ------- | ------- |
| MOSI   | PC3  | 22      | 3       |
| CLK    | PC5  | 24      | 4       |
| DC/RS  | PC1  | 20      | 5       |
| RST    | PA6  | 10      | 6       |
| CS     | PA1  | 2       | 7       |
| GND    | —    | GND     | 8 / 13  |
| VCC    | —    | 3.3V    | 10      |
| BL     | PB3  | 13      | 11 / 12 |

> **Note**: Backlight `PB3` HIGH = ON (direct drive, not via NPN transistor)

### Battery Sensing

| Signal     | GPIO | Notes                                      |
| ---------- | ---- | ------------------------------------------ |
| VBAT sense | PB1  | 1:4 resistor divider from raw battery rail |

> **Note**: The TLSR8232's internal VBAT ADC channel reads VDD (regulated 3.3V), not the battery.
> PB1 reads VBAT/4 via a hardware resistor divider on the PCB. Multiply ADC result by 4 to recover
> actual battery voltage (3.0–4.2V range). Verified by GPIO scan with known battery voltages.

### UART Debug (FPC pads)

| Signal | GPIO | Notes         |
| ------ | ---- | ------------- |
| TX     | PB4  | FPC debug pad |
| RX     | PB5  | FPC debug pad |

### PCB Test Pads

| Pad | Function                                 |
| --- | ---------------------------------------- |
| SWS | SWire debug interface (flash / recovery) |
| DAT | Secondary GPIO — not SWire               |
| 3V3 | 3.3V supply (post-LDO, not VBAT)         |
| GND | Ground                                   |

### Other Peripherals (not yet mapped)

> Vibrator, heart rate sensor, buttons, accelerometer — GPIO assignments unknown.
> ADC scan hinted PB2/PB5 may carry HR sensor signals but not confirmed.

---

## Project Structure

```
.
├── binaries/               # Precompiled firmware binaries
├── docs/                   # Datasheets and pinout diagrams
├── examples/               # Example firmware projects
│   ├── display/            # ST7735 display test
│   ├── display_lib/        # Shapes + text via the gfx layer
│   ├── image/              # Color images from flash (RLE, sprites, alpha masks)
│   ├── logo_splash/        # e-lab innovations boot splash
│   ├── battery_pct/        # Battery percentage readout
│   ├── uart/               # UART debug output
│   ├── ble_adv/            # BLE advertising
│   └── blink/              # Backlight blink
├── lib/                    # Reusable libraries
│   ├── display/            # ST7735 display driver
│   ├── fonts/              # Bitmap fonts (Adafruit GFX format)
│   └── uart/               # UART helper
├── sdk/                    # Docker-based build environment
└── tools/
    ├── img2c.py            # PNG → C header (BGR565, optional RLE / alpha mask)
    └── tlsr82-debugger-client/  # SWire flash tool
```

---

## Quick Start

### 1. Build the Docker SDK image

```bash
cd sdk
docker build -t tlsr8232-sdk .
```

### 2. Build an example

```bash
# mount the repo root — examples pull in ../../lib
docker run --rm -v $(pwd):/src -w /src/examples/display -it tlsr8232-sdk make
```

### 3. Flash to the watch

```bash
cd tools/tlsr82-debugger-client
python tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 \
  write_flash ../../examples/display/_build/display.bin
```

---

## Flashing — SWire Recovery

The TLSR8232 24-pin package has **no RST pin**, which makes SWire flashing tricky. The chip must be caught in SWire mode right at power-on.

### Hardware Required

- STM32F103C8 Blue Pill
- ST-Link V2
- 3 jumper wires

### Blue Pill Wiring

| Blue Pill            | Watch PCB pad |
| -------------------- | ------------- |
| A7 — via 1K resistor | SWS           |
| A6 — direct          | SWS           |
| GND                  | GND           |
| 3V3                  | 3V3           |

### Flash Blue Pill firmware

```bash
st-flash --format ihex write binaries/USB2Swire-STM32F103C8-v06.hex
```

Verify — `lsusb` should show:

```
iProduct: USB to SWire
```

### Connect to chip

Power the watch first, then immediately run:

```bash
python tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 \
  get_soc_id
# Expected: SOC ID: 0x5316
```

---

## OTA Flashing

When the watch has a working BLE firmware, you can flash OTA without any hardware:

1. Open [atc1441.github.io/BLE_EPaper_OTA.html](https://atc1441.github.io/BLE_EPaper_OTA.html) in Chrome or Edge
2. Click **Connect** and select the watch from the BLE device list
3. Select a `.bin` file
4. Click **Flash**

> ⚠️ OTA can soft-brick the watch if interrupted mid-flash. If this happens, use SWire recovery above.

---

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
