# Flashing

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
