# TLSR82 Debugger Client

Python script to interact with Telink TLSR82xx chips over SWire for flashing and debugging.

Originally developed by [rbaron](https://github.com/rbaron) for the [m6-reveng](https://github.com/rbaron/m6-reveng) project.

---

## Requirements

- STM32F103C8 Blue Pill flashed with `binaries/USB2Swire-STM32F103C8-v06.hex`
- Python 3.x

### Wiring (TLSR8232 24-pin — no RST pin)

| Blue Pill            | Watch pad |
| -------------------- | --------- |
| A7 — via 1K resistor | SWS       |
| A6 — direct          | SWS       |
| GND                  | GND       |
| 3V3                  | 3V3       |

> **Important**: The 24-pin TLSR8232 has no RST pin. Power the watch first, then immediately run the script to catch the chip in SWire mode at boot.

---

## Setup

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

---

## Usage

### Help

```bash
python tlsr82-debugger-client.py --help
```

### Get chip ID

```bash
python tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 \
  --debug \
  get_soc_id
# Expected output: SOC ID: 0x5316
```

### Flash firmware

```bash
python tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 \
  write_flash {path_to_firmware}.bin
```

### Dump flash from chip

```bash
python tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 \
  dump_flash output.bin
```

---

## Troubleshooting

| Symptom                              | Cause                          | Fix                                                  |
| ------------------------------------ | ------------------------------ | ---------------------------------------------------- |
| `lsusb` shows `USB to SPI Converter` | Wrong hex flashed to Blue Pill | Reflash with correct `USB2Swire-STM32F103C8-v06.hex` |
| `Unable to find suitable SPI speed`  | Chip not in SWire mode         | Power watch first, then immediately run script       |
| `ff ff ff ff ff ff` response         | Blue Pill not responding       | Check serial port, replug USB                        |
| `SOC ID: 0x5316` then flash fails    | Partial connection             | Check SWS wiring, ensure 1K resistor on A7           |
