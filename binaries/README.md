# Binaries

Precompiled firmware binaries used in this project.

---

### [ATC_Cheapo_BLE_OEPL_Watch.bin](ATC_Cheapo_BLE_OEPL_Watch.bin)

Custom BLE OEPL firmware for the TLSR8232-based watch, from the [OpenEPaperLink](https://github.com/OpenEPaperLink/OpenEPaperLink) project.

- **Source**: [OpenEPaperLink/binaries/Tag/ATC_Cheapo_BLE_OEPL_Watch.bin](https://github.com/OpenEPaperLink/OpenEPaperLink/blob/master/binaries/Tag/ATC_Cheapo_BLE_OEPL_Watch.bin)
- **Purpose**: First working custom firmware flashed to this watch. Used to verify the display and BLE stack work, and as a reference for reverse engineering the display pin mapping.
- **Flash via**: OTA (BLE) or SWire (recovery)

> ⚠️ OTA flashing this firmware on a watch with a corrupted bootloader will soft-brick it. Use SWire recovery if that happens.

---

### [USB2Swire-STM32F103C8-v06.hex](USB2Swire-STM32F103C8-v06.hex)

STM32F103C8 (Blue Pill) firmware that implements a USB-to-SWire bridge, enabling SWire flashing of the TLSR8232 from a PC.

- **Source**: [pvvx/TlsrTools](https://github.com/pvvx/TlsrTools/blob/master/STM32F103/USB2Swire-STM32F103C8-v06.hex)
- **Purpose**: Flash this to a Blue Pill via ST-Link, then use it with `tools/tlsr82-debugger-client` to read/write TLSR8232 flash over SWire.
- **Flash via**: ST-Link V2

```bash
st-flash --format ihex write USB2Swire-STM32F103C8-v06.hex
```

After flashing, verify with `lsusb` — should show `iProduct: USB to SWire`.
