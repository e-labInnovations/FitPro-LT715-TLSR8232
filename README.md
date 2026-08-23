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

### Pin Inventory — TLSR8232F512/F128 ET24

All 24 package pins. Only 16 GPIOs are bonded out; PA0, PA2, PA7, PB0, PB6, PB7,
PC0 and PC6 have no pad on this package.

| Pin | GPIO | Alt functions (datasheet)                   | Board function      |
| --- | ---- | ------------------------------------------- | ------------------- |
| 1   | PC7  | SWS / PWM3 / ANA_C7                         | SWS debug pad       |
| 2   | PA1  | PWM3_N / UART_CTS / ANA_A1                  | LCD CS              |
| 3   | PA3  | PWM4 / UART_TX / I2C_MCK / DI / ANA_A3      | **unknown**         |
| 4   | PA4  | PWM2 / UART_RX / I2C_MSD / CK / ANA_A4      | **unknown**         |
| 5   | —    | VDD3                                        | supply              |
| 6   | —    | DVSS                                        | ground              |
| 7   | —    | DVDDDEC                                     | supply              |
| 8   | —    | VDDDEC_F                                    | supply              |
| 9   | PA5  | PWM5 / I2C_CK / I2C_MCK / ANA_A5            | **unknown**         |
| 10  | PA6  | PWM4_N / I2C_SD / RX / CYC2LNA / ANA_A6     | LCD RST             |
| 11  | PB1  | PWM0 / MDO / TX / CYC2PA / ANA_B1 / pga_in0 | VBAT/4 sense        |
| 12  | PB2  | PWM2 / MDI / UART_CTS / ANA_B2 / pga_in1    | **unknown** (HR?)   |
| 13  | PB3  | PWM0 / MCK / UART_RTS / I2C_MSD / ANA_B3    | LCD backlight       |
| 14  | PB4  | PWM1_N / UART_TX / ANA_B4 / pga_in2         | UART TX pad         |
| 15  | PB5  | PWM4 / UART_RX / ANA_B5 / pga_in3           | UART RX pad (HR?)   |
| 16  | —    | XC2                                         | crystal             |
| 17  | —    | XC1                                         | crystal             |
| 18  | —    | AVDD3                                       | supply              |
| 19  | —    | ANT                                         | antenna             |
| 20  | PC1  | PWM2_N / ANA_C1                             | LCD DC/RS           |
| 21  | PC2  | CN / PWM0_N / MCN / UART_CTS / ANA_C2       | Touch key           |
| 22  | PC3  | DO / PWM5_N / MDO / UART_RTS / ANA_C3       | LCD MOSI            |
| 23  | PC4  | DI / I2C_MSD / UART_TX / ANA_C4             | **unknown**         |
| 24  | PC5  | CK / I2C_MCK / MCK / UART_RX / ANA_C5       | LCD CLK             |

Five pins are still unaccounted for — **PA3, PA4, PA5, PB2, PC4** — and the
vibrator motor, heart-rate sensor and accelerometer are still to place. PB4/PB5
are only *assumed* to be plain debug pads, so they are candidates too. See
[Finding Unknown Pins](#finding-unknown-pins).

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

### Touch Key

| Signal    | GPIO | MCU Pin | Notes                                              |
| --------- | ---- | ------- | -------------------------------------------------- |
| Touch key | PC2  | 21      | Active **HIGH**, idle low, level follows the finger |

> Characterised with the `touch_key` example: census class `L` (the pin is
> actively driven low at rest) and it goes high for exactly as long as the key is
> touched — so it is a push-pull touch-IC output, not a fixed-width pulse. Sense
> it **float**: no internal pull is needed, and a pull would only fight the
> driver. Driver: [lib/touch/](lib/touch/).

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

## Finding Unknown Pins

Two firmware examples hunt the remaining six pins (PA3, PA4, PA5, PB2, PC2, PC4).
Both report on the watch's own screen, so no probe wiring is needed; UART logging
on PB4/PB5 is optional and controlled by `ENABLE_UART` at the top of `main.c`.
The shared candidate list and the probe helpers live in [lib/pinscan/](lib/pinscan/).

### 1. `pin_probe` — touch key and other inputs

```bash
docker run --rm -v $(pwd):/src -w /src/examples/pin_probe -it tlsr8232-sdk make
```

**Stage 1 — census.** Each candidate is read as an input, first with the internal
10K pull-up and then with the internal 100K pull-down. The class column tells you
what is on the other side of the pin:

| Class | Meaning                                                       |
| ----- | ------------------------------------------------------------- |
| `F`   | follows the internal pull — high-Z, nothing driving it         |
| `H`   | stays high against the pull-down — driven high / hard pull-up  |
| `L`   | stays low against the pull-up — driven low / hard pull-down    |
| `?`   | inverted response — noise or bad contact, re-run              |

**Stage 2 — live monitor.** Cycles three sense modes, 10 s each, counting every
level change per pin. Tap and hold the watch face during each mode and watch
which row's `CNT` moves:

| Mode     | Catches                                                  |
| -------- | -------------------------------------------------------- |
| `PULLUP` | a switch or open-drain output that pulls the pin to GND   |
| `PULLDN` | a switch or output that pushes the pin to 3V3             |
| `FLOAT`  | an actively driven push-pull output (a capacitive touch IC) |

Screen columns are `PIN  C  L  CNT`. A row latches yellow on its first edge, so a
single quick tap is never lost between redraws.

### 2. `vib_hunt` — vibrator motor

```bash
docker run --rm -v $(pwd):/src -w /src/examples/vib_hunt -it tlsr8232-sdk make
```

Sweeps one candidate at a time, showing the pin name and phase on screen. Hold
the watch and note the name displayed when it buzzes. Each pin gets three phases,
with the pin left floating in between:

| Phase   | Drive                | Finds                                        |
| ------- | -------------------- | -------------------------------------------- |
| `HIGH`  | 3V3 for 400 ms       | active-high low-side driver (NPN/NMOS gate)  |
| `BLINK` | 4× on/off at ~4 Hz   | same, but the stutter separates buzz from knocks |
| `LOW`   | GND for 400 ms       | active-low high-side driver (PNP/PMOS gate)  |

Pins the census finds externally driven (`H` / `L`) are **skipped** — driving
against another IC's output means current through both drivers. `FORCE_ALL 1`
overrides this; the pulses stay short, but check the census result first. Once a
pin buzzes, set `ONLY_INDEX` to its row number to loop that pin alone.

Both examples park every candidate as high-Z before touching the display, so a
motor pin is never left enabled while the census runs.

### 3. `touch_key` — PC2 only

```bash
docker run --rm -v $(pwd):/src -w /src/examples/touch_key -it tlsr8232-sdk make
```

`pin_probe` pinned the touch key to PC2, so this one watches only that pin and
reports what a driver needs: sense mode, measured idle level, debounced
press/release with press and long-press counters, and the duration of the last
press. The bottom half draws the raw undebounced level as a sweeping trace
(1 px per 8 ms sample, ~1 s per sweep), which tells the two output styles apart:

- **held level** — trace follows your finger, `last` grows with the hold
- **fixed pulse** — narrow spike, `last` stays the same no matter how long you hold

`FORCE_MODE` overrides the auto-picked sense mode (0=pull-up, 1=pull-down,
2=float) if the auto choice reads the wrong idle level.

Result on the LT716: `FL L idle0` — driven low at rest, high while touched, and
`last` grows with the hold. That became [lib/touch/](lib/touch/).

### Using the touch key

[lib/touch/touch.h](lib/touch/touch.h) turns PC2 into a UI button: 24 ms
debounce, and one event per `touch_poll()` call.

```c
touch_init();                       // safe to call before display_init
while (1) {
    switch (touch_poll()) {         // poll at least every ~10 ms
        case TOUCH_TAP:        next_screen();          break;
        case TOUCH_DOUBLE_TAP: toggle_units();         break;
        case TOUCH_LONG_PRESS: backlight_off();        break;  // fires while held
        default: break;
    }
    sleep_ms(5);
}
```

A long press reports `TOUCH_LONG_PRESS` while the finger is still down and then
`TOUCH_UP` on release, so it never also reads as a tap. `touch_is_down()` and
`touch_press_ms()` give the live state and hold time.

Timing knobs — `TOUCH_LONG_MS` (1400 ms), `TOUCH_DTAP_MS` (500 ms),
`TOUCH_DEBOUNCE_MS` (24 ms). Each is `#ifndef`-guarded, so an app can override it
by `#define`-ing before including the header. The long-press threshold is
deliberately high: this key's IC holds the line up after the finger leaves, so an
ordinary tap already measures 400–550 ms and anything near 600 ms would make
every normal touch read as a long press.

Working demo — tap / double-tap counters, live hold time, long press toggles the
backlight:

```bash
docker run --rm -v $(pwd):/src -w /src/examples/touch_demo -it tlsr8232-sdk make
```

### 4. `vib_sweep` — vibrator motor, second pass

```bash
docker run --rm -v $(pwd):/src -w /src/examples/vib_sweep -it tlsr8232-sdk make
```

`vib_hunt` drove DC and a 4 Hz blink and found nothing. This closes the four
remaining gaps:

| Gap | Covered by |
| --- | ---------- |
| An LRA does not move on DC — it needs ~150–250 Hz | frequency sweep: 50, 100, 150, 175, 205, 235, 300 Hz |
| Census-skipped pins (a gate with a bleed resistor reads as `L`) | every candidate is driven; the class is logged, not obeyed |
| PB4/PB5 never swept | `ENABLE_UART 0` |
| A driver that needs a second pin held high first | `PAIR_PASS` holds each pin high while pulsing every other |

Tap the touch key to skip ahead during the pause between pins. The pair pass is
the expensive one: 6 pins → 30 ordered pairs.

Ignoring the census is the deliberate risk here — if a pin turns out to be
another IC's output, driving it puts current through both drivers. Every drive is
short and the pin returns to high-Z between steps, but don't leave it running
unattended.

### 5. `vib_probe` — vibrator hunt by battery sag

```bash
docker run --rm -v $(pwd):/src -w /src/examples/vib_probe -it tlsr8232-sdk make
```

The other two sweeps need a human to feel a buzz. This one measures instead: a
spinning motor pulls tens of mA, which sags the battery rail, and PB1 already
reads VBAT/4 through the PCB divider. So drive each candidate — DC, then 205 Hz —
and watch what the load does to the rail.

Columns are `PIN`, largest DC sag in mV, largest AC sag in mV, both held at their
maximum across passes so a one-off event is not lost. The rail's own noise is a
few mV; a row goes yellow at 15 mV and green at 45 mV.

It is strictly more sensitive than feeling for a buzz:

- catches a motor too weak, too brief, or too well damped to feel
- tells "no motor on any pin" apart from "motor leads are off" — with the leads
  off, nothing sags anywhere, including the pin that is really the motor's
- also catches contention: a pin fighting another IC's output draws current too,
  which is worth knowing before driving it any longer

**Run it on the LiPo.** A bench supply or USB LDO regulates the sag away and
every column stays at zero.

### If the motor never shows up

Worth confirming the motor works at all before trusting any negative result:
bridge its two pads to 3V3 through ~100 Ω by hand. If it does not spin, no
firmware sweep ever will.

Failing that, the multimeter route is quicker than more sweeps. The motor is
driven by a small transistor next to it (SOT-23/SOT-323, usually with a flyback
diode across the motor pads). Beep from the transistor's base/gate — or the far
end of its series resistor — to each candidate package pin: 3 (PA3), 4 (PA4),
9 (PA5), 12 (PB2), 23 (PC4). Exactly one will read as a short.

### Watching the stock firmware instead

Guessing pins from the outside is the slow way round. SWire reads the bus
independently of the CPU, so with the **stock** firmware running you can watch
which pin its own code drives. This needs a watch that still has its stock
firmware — once you have flashed over it, this route is closed until you get
another one, and the dumps in this repo are not a substitute (see below):

```bash
# 1. re-dump the stock flash, then prove the dump is faithful
python tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 dump_flash binaries/stock/LT716_v2.bin
python tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 verify_flash binaries/stock/LT716_v2.bin

# 2. flash the stock image back (only once verify_flash reports a match)
python tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 write_flash binaries/stock/LT716_v2.bin

# 3. resume the chip and watch its GPIO registers
python tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 watch_gpio
```

Then make the watch vibrate — the FitPro app can do it on command, which makes
this a repeatable experiment rather than a wait for an alarm.

Narrow the watch to the pins still in question, so nothing else can distract:

```bash
python tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 watch_gpio --only PA3,PA4,PA5,PB2,PC4
```

By default the display bus and SWS (`PA1,PA6,PB3,PC1,PC3,PC5,PC7`) are ignored —
they toggle on every frame the stock UI draws and would bury everything else.
`--only` overrides that with an explicit list.

Every `OEN`/`OUT` transition is printed with a timestamp, pins already mapped are
labelled so an unexplained one stands out, and Ctrl-C prints a summary ranked by
transition count. **OEN is active low** — a `0` bit means the driver is on. Two
passes are worth running: `--regs OEN` alone is very quiet, since a pin switching
from high-Z to driven is exactly what a motor turning on looks like; `--regs OUT`
catches a pin that is always an output and merely changes level.

Wiring: SWS and GND to the bridge, and leave the bridge's 3V3 **disconnected**
while the watch runs on its own battery. `init_soc` resets the chip on connect,
so the firmware reboots and the app has to reconnect before you trigger the buzz.

For the disassembly route, the Ghidra project in `ghidra_project/` already has
`GpioPinMap.java`, which decodes stores to the GPIO registers — but re-dump
first, for the reason below.

### Stock firmware images

| File | Model | Version | Build stamp | Verdict |
| ---- | ----- | ------- | ----------- | ------- |
| `LT716_V10712_211091429.bin` | `LT716(G)` | `V10712` | `211091429` → 2022-11-09 14:29 | good — flashed and running |
| *(deleted)* `LT716-2.bin` | `LT716(G)` | `V16096` | `307211745` → 2023-07-21 17:45 | was corrupt; recoverable from commit `67407fd` if ever needed |

The version string lives at `0x019054` (`0x019258` in the older dump), inside
identical surrounding code — same firmware family, two builds. `V10712` <
`V16096` agrees with the build stamps: the newly bought watches ship an *older*
build than the watch dumped earlier.

The stock boot screen prints all of it — model, version, short name, a per-unit
ID, and the build stamp:

```
LT716(G)
V10712
716
385C90B5      <- not present anywhere in flash; generated at runtime
211091429
```

The build stamp sits right after the model string, `YMMDDHHMM` for year `202Y`.
One sample alone is ambiguous — `211091429` also reads as 2021-10-09 — but
`307211745` only parses under `YMMDDHHMM`, since `30` is not a month. So the
newly bought watches carry a **2022-11-09** build, *older* than the 2023-07-21
one dumped previously.

`LT716_V10712_211091429.bin` checks out as a faithful read. It was dumped twice;
the two dumps were byte-identical (`md5 a52d9e46df2ed00704a427c82fecd22c`), so the
second copy was dropped rather than kept. It has since been flashed to a second
watch, which boots and advertises normally.

- two independent dumps came back identical, so the read is reproducible
- the body ends exactly at the length declared at offset `0x18` (`0x1a8a4`), to
  the byte
- all 776 code-pointer-shaped words in the body land inside it; none dangle
- shared markers at `0x0584e1` and `0x0594d0` sat at the *same absolute offset*
  in this dump and in both older ones, so there is no cumulative byte drift
- every erased run ends on a 4 KB sector boundary

### ⚠️ …but the dump stops short of the chip

`FLASH_SIZE` in the client is `0x7d000`, which is what the firmware occupies —
not what the part holds. A TLSR8232F512 is `0x80000`, so **the top 12 288 bytes
(`0x7d000`–`0x80000`) were never read.**

That matters: there is no BLE MAC anywhere in the dumped range. `CFG_ADR_MAC` for
a 512 KB layout is `0x76000`, which reads as all-`ff` here, and no Telink OUI
appears anywhere in the image. So the MAC is either in the undumped top or
derived at runtime.

Evidence for the latter: writing this image to a *different* watch works, and
that watch keeps its own identity — three units advertising at once show three
distinct addresses (`4C:00:77:24:35:A2`, `F0:57:3A:F0:7A:4A`,
`AE:18:2F:73:33:FE`), none of them a Telink OUI, which is what randomly
generated static addresses look like. The per-unit ID on the boot screen is not
in flash either. **So the image is portable between units** — flashing it does
not clone one watch's identity onto another.

Even so, `erase_flash` is a whole-chip erase, and nothing above `0x7d000` has
ever been read to know what it destroys. Capture the rest before erasing:

```bash
# what size is the flash, really?
python tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 flash_id

# grab whatever sits above the old ceiling
python tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 dump_flash top.bin --start 0x7d000 --length 0x3000
```

### ⚠️ What the older, deleted dumps taught us

`LT716.bin` and `LT716-2.bin` were two dumps of the same chip and they disagreed
in 14 379 bytes across 1951 runs. The differing regions are not different data —
they are *the same data at a small offset*: at `0x500` the second dump's bytes
appear in the first at `+10`, at `0x900` at `+6`, at `0xa40` at `+24`.

Ten bytes is exactly the reply truncation documented at the top of
`tlsr82-debugger-client.py` (`2 x (4 header + 1 trailer)`). Both dumps were taken
with the old sleep-based reply reader, lost bytes mid-stream, and re-synced — so
everything after each loss is shifted. The header and length still look right,
which is why the file appears valid and will not run.

`LT716_main.bin` was byte-identical to `LT716.bin[:0x40000]` and `LT716_ota.bin`
to `LT716.bin[0x40000:0x7c000]` — carved slices of the same corrupt image, with
no independent content — while `LT716_nvs.bin` was 4 KB of `0xff`.

All of them are deleted, `LT716-2.bin` included. A dump with a sliding stream is
no use for flashing and worse than useless for analysis, and the only facts worth
keeping from it — build `V16096`, stamp `307211745`, and the truncation
fingerprint above — are recorded here. The files remain in git history at
`67407fd` if a reason to look at them ever appears.

A register-literal scan of the verified dump finds every GPIO port register
individually (`PA_OEN` 15 references, `PA_OUT` 15, `PC_OEN`/`PC_OUT` 10 each,
`PB` only 4/3) and, of the PWM block, only `pwm_enable` once — no `pwm_cycle`,
`pwm_cmp` or `pwm_phase` anywhere. That would suggest the stock firmware never
sets up a PWM channel and drives the motor as a plain GPIO.

Treat that as inconclusive, though: the literal `0x800000` appears 71 times, so
plenty of register access goes through a base pointer plus an offset, which a
literal scan cannot see. The absence of PWM literals is suggestive, not proof —
which is why `vib_sweep` still sweeps frequencies.

Commit `8e833ae` replaced the sleep-based reader with one that reads replies by
length, so a fresh `dump_flash` should be clean. Confirm with `verify_flash`
before trusting or flashing it.

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
│   ├── color_test/         # RGB565 channel-mapping diagnostic
│   ├── battery_pct/        # Battery percentage readout
│   ├── pin_probe/          # Unknown-pin input census + touch key hunt
│   ├── vib_hunt/           # Unknown-pin output sweep (vibrator motor hunt)
│   ├── vib_sweep/          # Vibrator hunt pass 2: AC sweep + pin pairs
│   ├── vib_probe/          # Vibrator hunt by battery sag (no buzz needed)
│   ├── touch_key/          # PC2 touch key characterisation + logic trace
│   ├── touch_demo/         # Touch driver demo: tap / double tap / long press
│   ├── uart/               # UART debug output
│   ├── ble_adv/            # BLE advertising
│   └── blink/              # Backlight blink
├── lib/                    # Reusable libraries
│   ├── display/            # ST7735 display driver
│   ├── fonts/              # Bitmap fonts (Adafruit GFX format)
│   ├── pinscan/            # Unknown-pin candidate list + probe/drive helpers
│   ├── touch/              # PC2 touch key driver (debounce, tap/long/double)
│   └── uart/               # UART helper
├── sdk/                    # Docker-based build environment
└── tools/
    ├── img2c.py            # PNG → C header (RGB565, optional RLE / alpha mask)
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
