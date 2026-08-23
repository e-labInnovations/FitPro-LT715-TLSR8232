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
| 4   | PA4  | PWM2 / UART_RX / I2C_MSD / CK / ANA_A4      | IRQ input (accel?)  |
| 5   | —    | VDD3                                        | supply              |
| 6   | —    | DVSS                                        | ground              |
| 7   | —    | DVDDDEC                                     | supply              |
| 8   | —    | VDDDEC_F                                    | supply              |
| 9   | PA5  | PWM5 / I2C_CK / I2C_MCK / ANA_A5            | **Vibrator motor**  |
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

Three pins are still unaccounted for — **PA3, PB2, PC4** — and the heart-rate
sensor is still to place. The vibrator (PA5) and an interrupt input that is
probably the accelerometer (PA4) came out of decompiling the stock firmware, not
out of pin sweeps: see [Reading the pins out of the firmware](#reading-the-pins-out-of-the-firmware).

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

> The stock firmware's own gauge, thresholds and charger detection are decoded in
> *Battery and charging, out of the firmware* below — including **PB2 as the
> charger sense input**, which the pin sweeps never found.

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

### Vibrator Motor

| Signal  | GPIO | MCU Pin | Notes                        |
| ------- | ---- | ------- | ---------------------------- |
| Motor   | PA5  | 9       | Active **HIGH**, plain GPIO  |

> Found by decompiling the stock firmware. `FUN_0000cdc4` toggles `PA_OUT` bit 5
> on an even/odd counter and stops once the counter passes a limit of 4, 8 or
> 0x18 — three buzz patterns chosen by alert type. The BLE command handler at
> `FUN_0000216c` clears the same bit when the app cancels, and three further
> functions only ever clear it. The toggle interval is a constant `0x493e0`
> compared as `<< 4` against the system tick at `0x800740`, i.e. a **300 ms half
> period**. Driver: [lib/vibrate/](lib/vibrate/), confirmed on hardware by
> [examples/vibrate/](examples/vibrate/) and [examples/vib_pa5/](examples/vib_pa5/).

> #### ⚠️ The motor only runs on battery power
>
> `examples/vibrate` appeared to do nothing for a while: the screen cycled
> through SHORT/MEDIUM/LONG but the motor never moved. The firmware was right
> the whole time — the board was on an external 3.3 V supply. A vibrator pulls
> tens of milliamps in an inrush the bench supply and the on-board LDO path will
> not deliver, so the motor sits still while every GPIO reads correct. Reconnect
> the LiPo and the same unmodified binary buzzes.
>
> This applies to any current-hungry load, so **test motors, backlight brightness
> and radio transmit on the battery**, never on 3V3 alone. It also explains why
> [examples/vib_probe/](examples/vib_probe/) insists on the LiPo: on a stiff
> supply the battery-sag column stays at zero no matter which pin is right.

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

> Heart rate sensor and accelerometer — GPIO assignments unknown. PA4 is an
> interrupt input and is the strongest accelerometer candidate (see *Reading the
> pins out of the firmware*). Now mapped: the vibrator (PA5), the touch key
> (PC2), the charger sense input (PB2) and PB5, which the stock firmware drives
> from the charger state — see *Battery and charging, out of the firmware*.

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

> **On the battery, always.** `vib_hunt` and `vib_sweep` both missed PA5 while
> the watch was on an external 3.3 V supply — the pin was driven correctly and
> the motor simply could not draw enough current to turn. See *If the motor never
> shows up*.

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

**Check the power source first.** This is what actually happened here: `vib_hunt`
and `vib_sweep` drove the right pin, PA5, and reported nothing — the watch was
running off an external 3.3 V supply, which cannot deliver the motor's inrush.
Every GPIO read correct and the rotor never moved. On the LiPo, the same
binaries buzz. A negative sweep result on bench power means nothing at all.

Then confirm the motor works: bridge its two pads to 3V3 through ~100 Ω by hand.
If it does not spin, no firmware sweep ever will.

Failing that, the multimeter route is quicker than more sweeps. The motor is
driven by a small transistor next to it (SOT-23/SOT-323, usually with a flyback
diode across the motor pads). Beep from the transistor's base/gate — or the far
end of its series resistor — to each candidate package pin: 3 (PA3), 4 (PA4),
9 (PA5), 12 (PB2), 23 (PC4). Exactly one will read as a short.

## Stock Firmware Structure

Reverse engineered from `binaries/stock/LT716_V10712_211091429.bin` with
[tools/fwtool.py](tools/fwtool.py). Every finding below was verified by decoding
it and rendering the result back out — the fonts read as legible text, the CJK
index matches Unicode, the strings match the UI.

```
0x000000-0x01a8a4  main application     Telink KNLT image, length at 0x18
0x01a8a4-0x020000  erased padding
0x020000-0x028c74  second application   KNLT image, 36 KB
0x028c74-0x029000  erased padding
0x029011-0x031c58  font, U+0001..U+07CB 1995 glyphs
0x031c58-0x0339b0  CJK codepoint index  3756 x uint16, sorted
0x0339b0-0x0441c8  font, CJK            3756 glyphs, same format
0x0441c8-0x044200  string table header  magic 0xBEEF
0x044200-0x04a860  UI strings           7 languages x 78 slots
0x04a860-0x06fc04  1bpp graphics        battery icons first, 56 px wide
0x06fc04-0x07d000  erased
```

**Two applications, not one.** Both carry the `KNLT` magic at +8 and their own
length at +0x18, and both end exactly at that length. The second is a third the
size of the first; the `UPGRADE` string in the main image suggests it is the OTA
updater, but that is not yet confirmed.

### Glyph format

Both fonts use one format: a **12x12 cell in 18 bytes**, split into two planes.

```
bytes 0..11   left 8 columns, one byte per row, MSB = leftmost pixel
bytes 12..17  right 4 columns, two rows per byte,
              high nibble = even row, low nibble = odd row
```

Latin glyphs never touch the right-hand plane, so those six bytes are zero — a
plain 8-wide, 18-row read renders them correctly by accident, which is a good
way to waste an afternoon. CJK glyphs use all twelve columns, and only the
split-plane read produces legible characters.

Lookup differs per range:

- **U+0001..U+07CB** — direct: `addr = 0x028fff + codepoint * 18`. Covers ASCII,
  Latin-1, Latin Ext-A/B, Greek, Cyrillic, Armenian, Hebrew, Arabic and Syriac,
  every block fully inked.
- **CJK** — binary search the sorted index at `0x031c58` (3756 entries,
  U+4E00..U+9F9F), then `addr = 0x0339b0 + index * 18`.

### UI strings

A flat table at `0x044200`: 48-byte slots holding UTF-16LE, 78 slots per
language, 7 language blocks of `0xea0` bytes each — English, Dutch, French,
Portuguese, Turkish, Malay, Slovak. Slot 0 is "Steps", slot 1 "Heart rate".

The slot list doubles as a feature inventory: Steps, Heart rate, Sports,
Running, Situp, Skipping, Bike riding, Camera, Stopwatch, Looking for, Message,
Qr code, Music, Sleep, Weather, Long press, Low battery, Power down, Unbundle.

### Graphics

The last 149 KB is **1bpp bitmaps**, and the reason it looked like noise for so
long is that the row width is not 128. Nothing stores the width, but rows of the
same shape repeat, so autocorrelation recovers it:

```bash
python3 tools/fwtool.py $FW width 0x04a860     # -> 7 bytes = 56 px
python3 tools/fwtool.py $FW bitmap icons.png --off 0x04a860 --width 7 --rows 180
```

At `0x04a860` that gives the battery icons — rounded outlines with the terminal
nub, one frame per charge level. Widths differ per asset: autocorrelation around
`0x05a000` points at 31 bytes instead of 7, so the region is a series of images
of assorted sizes rather than one uniform sheet.

Still open: the per-asset index. Offsets and dimensions are presumably passed
from code, so finding them properly means disassembling — see below. There is no
sign of an offset table in the region itself.

### Getting to code

Not with capstone. TC32 is Telink's own ISA, and while it looks Thumb-shaped it
does not share Thumb's encodings — 108 KB of TC32 code contains 349 byte pairs
that would be a Thumb `push`, 33 that would be a `pop`, and **zero** `bx lr`.
Real Thumb code that size would show thousands of each. Feeding it to capstone
in Thumb mode produces long runs of implausible `lsrs`, which is what data looks
like when you force a decoder onto it.

The route that does work is Ghidra with a TC32 processor module, which is what
`ghidra_project/` is set up for. Two things make that easier now:

- `fwtool.py map` gives the exact code/data boundaries, so the fonts, string
  table and bitmaps can be marked as data instead of being disassembled into
  noise. Only `0x000000-0x01a8a4` and `0x020000-0x028c74` are code.
- `GpioPinMap.java` already decodes stores to the GPIO registers, and a literal
  scan confirms the registers are referenced individually — `PA_OEN` and
  `PA_OUT` 15 times each, `PC_OEN`/`PC_OUT` 10 each, `PB` only 4/3.

### Decompiling the whole image to C

Yes — the whole image, not just the parts you have questions about.
`DecompAll.java` walks every defined function and writes its decompiled body to
one file:

```bash
export JAVA_HOME=/path/to/jdk21     # Ghidra 12 needs 21+
ghidra/support/analyzeHeadless <project-dir> LT715_RE \
  -process "LT716_V10712*" -noanalysis \
  -scriptPath ghidra_project -postScript DecompAll.java
# writes /tmp/firmware_decomp.c
```

The result is checked in:
[ghidra_project/decompiled/LT716_V10712_211091429.c](ghidra_project/decompiled/LT716_V10712_211091429.c)
— **1039 of 1040 functions, 33 000 lines**, one decompile failure (a `pcode
error` at `0x16608`, a single unresolved constructor in the TC32 module).

Be clear about what that file is and is not:

- It **is** real C, per function, with control flow, loops and arithmetic
  recovered, and it is searchable — which is how the vibrator, the tick source
  and the buzz timing were all found in minutes instead of hours.
- It is **not** buildable source. There are no symbol names (`FUN_0000cdc4`), no
  types, no struct layouts, and every hardware register appears as a store
  through an opaque pointer (`*PTR_DAT_0000ceb4 = ... | 0x20`) because the
  register space is not part of the image. Global variables are addresses.
- Getting from there to source you could compile means naming things and
  rebuilding the types by hand, function by function. Practical for a subsystem
  you care about; a large project for 1040 functions.

### As a buildable project

[reversed/](reversed/) takes that file the rest of the way: generated headers, a
Makefile, and C the compiler accepts.

```bash
docker run --rm -v $(pwd):/src -w /src/reversed -it tlsr8232-sdk make
# compiled 1040 functions
#  184652 bytes of text, zero errors
```

`tools/decomp2proj.py` does it mechanically, and the part that matters is
resolving the symbols Ghidra leaves opaque. Every `DAT_` is a literal-pool word
inside the image, so its value can just be read out of the binary:
`PTR_DAT_0000ceb4` is the word at `0xceb4`, which is `0x800583`, which is
`PA_OUT`. Substituting the 343 dereferences that land on a known register turns

```c
*PTR_DAT_0000ceb4 = *PTR_DAT_0000ceb4 | 0x20;
```

into

```c
PA_OUT = PA_OUT | 0x20;
```

**It compiles; it will never run** — and [reversed/README.md](reversed/README.md)
is blunt about why. The image is full of pointers to its own code (jump tables,
callback arrays), and recompiling moves every function, so each of those now aims
somewhere meaningless. The fonts, strings and bitmaps it references are not in
the project either. There is deliberately no target that emits a `.bin`.

Places where the recovery is incomplete are marked rather than hidden:
`FW_UNKNOWN_ARG` (154 — a call site with fewer arguments than the signature),
`FW_VOID_RESULT` (205 — a value read from a function recovered as returning
none), `dropped:` comments (264 — surplus arguments kept instead of deleted).

So the workflow that pays off is not "decompile everything and read it" but
"decompile everything, then grep it": find the register write, follow it to the
task, follow the task to its caller, and name only that path. A helper for
pulling one function out of the dump:

```bash
awk -v n=FUN_0000cdc4 '$0 ~ "^/\\* ======== "n" @" {p=1}
  p && /^\/\* ======== / && $0 !~ "^/\\* ======== "n" @" {exit} p' \
  ghidra_project/decompiled/LT716_V10712_211091429.c
```

### Using the tool

```bash
FW=binaries/stock/LT716_V10712_211091429.bin
python3 tools/fwtool.py $FW map                 # the layout above
python3 tools/fwtool.py $FW glyph U+4E09 A 上   # ASCII art for any codepoint
python3 tools/fwtool.py $FW strings             # every string, all languages
python3 tools/fwtool.py $FW cjk --count 128     # the codepoint index
python3 tools/fwtool.py $FW atlas out.png --cjk --count 96 --cols 24
```

---

## Reading the pins out of the firmware

The pin sweeps never found the motor. Decompiling the stock firmware did, in one
sitting — and it turns out **Ghidra 12 ships a `Telink_TC32` processor module**,
so no community plugin is needed. Disassembly of the main application yields
34 729 instructions, and analysis of the full image finds 1040 functions — the
decompiler produces clean C for 1039 of them.

The scripts are in [ghidra_project/](ghidra_project/):

| Script | What it does |
| ------ | ------------ |
| `SetupBlocks.java` | run as `-preScript` on import: adds the register and SRAM blocks, marks both KNLT entry points |
| `GpioMapAll.java` | one table of every GPIO access in the image — function → registers → pin bits |
| `GpioRefs.java` | same idea for one port, but prints the disassembly window and C for each hit |
| `Pa5Sites.java` | disassembly around every PA access, flagging bit-5 masks — how the motor pin was pinned down |
| `DecompAll.java` | decompiles every function to one C file (see above) |
| `GpioPins.java`, `DumpFuncs.java` | earlier, narrower versions of the above |

Import and analyse the **whole** verified image, not a code-only slice:

```bash
export JAVA_HOME=/path/to/jdk21
FW=binaries/stock/LT716_V10712_211091429.bin
ghidra/support/analyzeHeadless $PWD/ghidra_project LT715_RE -import $PWD/$FW \
  -processor "Telink_TC32:LE:16:default" \
  -loader BinaryLoader -loader-baseAddr 0x0 \
  -scriptPath $PWD/ghidra_project -preScript SetupBlocks.java
```

Then any analysis script against that program:

```bash
ghidra/support/analyzeHeadless $PWD/ghidra_project LT715_RE \
  -process "LT716_V10712*" -noanalysis \
  -scriptPath $PWD/ghidra_project -postScript GpioMapAll.java
```

`GpioMapAll.java` prints the entire pin map in one pass — 81 register literals,
107 references, 41 functions:

```
FUN_0000cdc4@0000cdc4   {PA_OUT[bit5/0,1]=1, PA_OUT[bit5]=3}      <- vibrator
FUN_00005958@00005958   {PB_OUT[bit3/0,4]=1, PB_OUT[bit3]=1}      <- backlight
FUN_000061a8@000061a8   {PA_OEN[bit1]=1, PA_OUT[bit1]=2, ...}     <- LCD CS
FUN_000066e8@000066e8   {PA_OEN[bit4]=1, PA_POL[bit4]=1}          <- IRQ input
FUN_0000c37c@0000c37c   {PB_OUT[bit5/0]=2, PB_OUT[bit5]=1}        <- PB5, purpose unclear
FUN_0000ed4c@0000ed4c   {PA_OEN[...], PA_DS[...], PA_FUNC[...]}   <- bulk init, ignore
```

#### ⚠️ Two setup mistakes that waste a day

Both of these were live in this repo's Ghidra project until they were caught, and
both produce analysis that looks completely plausible:

- **Import at base 0, not `0x20000000`.** TC32 fetches from flash mapped at 0, so
  the image belongs there. With the image at `0x20000000`, every code pointer
  stored *inside* the image still reads `0x0000xxxx` and resolves to nothing, so
  cross-references silently vanish. `SetupBlocks.java` also has to create the
  register space (`0x800000`) and SRAM (`0x840000`) as uninitialized blocks
  before analysis, or the GPIO literals point into a hole.
- **Check which file the database was actually built from.** This project's
  original database was imported from an early, corrupt dump — `md5
  f36a2b58…`, not the verified `a52d9e46…`. Same KNLT header, same 787 decodable
  functions, and the vibrator conclusion happened to survive; other offsets did
  not. Verify with:

  ```java
  println(currentProgram.getExecutableMD5());   // compare against md5sum of your dump
  ```

  Any conclusion drawn from a database built on an unverified dump has to be
  re-derived after re-importing the good one. See *What the older, deleted dumps
  taught us*.

### Why the sweeps missed it

The register accesses that matter are single-bit writes through a literal:
`*DAT_0000ceb4 = *DAT_0000ceb4 | 0x20`. Two things drown that out if you are not
careful, and both cost me time:

- **Matching decompiled text for `0x800583` finds nothing.** The address lives in
  a literal pool, so the decompiler prints the literal's symbol
  (`DAT_0000ceb4`), not the constant. You have to map the literal back yourself.
- **The SDK's own GPIO helpers take the pin as a parameter**, so their masks are
  variables and they appear to touch every pin at once. `FUN_0000f098`,
  `FUN_0000f05c` and `FUN_0000f158` are those; ignore them. Bulk inits like
  `*DAT_0000edd8 = 0xdf` are the same kind of noise.

What is left is short and unambiguous:

| Pin | Evidence | Conclusion |
| --- | -------- | ---------- |
| PA5 | `\| 0x20` / `& 0xdf` in four functions, one of them a toggle-counter with limits 4/8/0x18 | **vibrator motor — confirmed on hardware** (on battery; see the warning under *Vibrator Motor*) |
| PA4 | `FUN_000066e8` sets FUNC/IE, disables the output driver, clears POL, sets an interrupt-enable bit and registers a handler | interrupt input, probably the accelerometer |
| PB5 | `FUN_0000c37c` drives it from the charger state | **driven from charging**, so the "UART RX pad" label is an assumption, not a measurement |
| PB2 | `FUN_0000c37c` reads ADC channel `0x104` and range-tests 4400–6500 mV | **charger sense input** — see *Battery and charging, out of the firmware* |
| PA3, PC4 | bulk init only | probably unused |

---

## The stock firmware's lifecycle

Screen off until you touch, one tap per screen, long press to select, off again
after a few seconds — all of it is in the decompilation, and the timings are
exact constants rather than guesses.

### The loop has two halves

`FUN_00004464` is the 10 ms main tick. It calls `FUN_0000e170`, which branches on
whether the watch is awake (`FUN_0000af38`, a single state byte):

| Awake | Asleep |
| ----- | ------ |
| `FUN_0000da74` UI/screen manager, `FUN_0000dd4c`, `vibrate_task`, sensor and BLE tasks | `FUN_0000cc54` only |

**Asleep is not suspended.** The CPU keeps running the whole time: step counting,
the battery read, the 1 Hz charger poll and BLE all continue. "Sleep" here means
the panel is off, nothing more — which matters for any power work, because there
is no deep-sleep state to wake from.

### Screen off

`FUN_0000a6e8` is the timeout test, and it reads a settings byte:

| Setting | Timeout |
| ------- | ------- |
| default | **5 s** since last activity |
| flag set | **15 s** |

When it expires, `FUN_0000d258` calls `FUN_00005958(0)`, which is the ST7735
shutdown in order: `0x28` DISPOFF, `0x10` SLPIN, then backlight PB3 low.

### Waking

PC2 is **interrupt-driven**, not polled. `FUN_00003b14(0x204, 1)` arms it at boot
(`0x204` = port 2, bit 2 = PC2, the SDK's own pin encoding). On each edge
`FUN_00003c60` clears GPIO IRQ bit `0x200000`, **flips the pin polarity** so the
opposite edge fires next, and hands the event to `FUN_0000bea4(key, state, tick)`.

If the watch was asleep, `FUN_0000d544` runs the wake sequence: `0x11` SLPOUT,
`0x28`, `0x13`, `0x29` DISPON, backlight on, then a full redraw. **The touch that
wakes it does not also advance a screen** — waking consumes the event.

### Tap versus long press

`FUN_0000bea4` stores the press timestamp in the key's slot with the low bit set
as a valid marker. What happens next is decided purely by how long you hold:

| Held for | Result | Constant |
| -------- | ------ | -------- |
| < 50 ms | ignored — debounce floor | `0xc350` |
| 50 ms … 1 s, then released | **tap**: current screen ← queued screen id (`+0x13` copied to `+0xf`), redraw flag set | `0xf4240` |
| still held at 1 s | **long press**: the UI tick calls `FUN_0000d778(1)` → `FUN_0000bbb0(1)`, which walks the screen table and enters the item | `0xf4240` |

So the stock long-press threshold is **1 s**, and a release after 1 s is
discarded rather than treated as a tap — which is why a slow finger on this
hardware feels like it does nothing. Our own driver uses 1400 ms
([lib/touch](lib/touch/)) for exactly that reason. Note there is **no double-tap
gesture anywhere in the stock firmware**.

### The screens

Screen ids are decimal-coded — `0x64` = 100, `0x136` = 310, `0x1f4` = 500 — as
`page * 100 + item`. Two tables:

- **0x019f40**, the carousel `FUN_0000bbb0` walks: 310, 510, 760, 741, 431, 421,
  411, 710, 610, 720 — ten entries, and the ASCII "Incoming Call" that follows in
  flash is what marks the end of it.
- **0x019e98**: the page roots and their items — 100, 200, 300, 400, 500, 600,
  700 among them.

Current screen lives at state `+0xf/+0x10`, the queued one at `+0x13/+0x14`, with
a redraw flag at `+0x1b`. `FUN_0000bdb8(id)` jumps straight to a screen; boot
calls it with **100**, the watch face.

### Everything else that wakes it

Touch is only one path. `FUN_0000d5d8` handles a notification — it sets the
vibrate flag *then* wakes the screen — and the charger-insert edge and an
incoming call each wake it the same way.

### Every timing constant in one place

| What | Value | Where |
| ---- | ----- | ----- |
| main tick | 10 ms | `DAT_000044d8` |
| touch debounce floor | 50 ms | `DAT_0000c1c8` |
| tap window / long-press threshold | 1 s | `DAT_0000c1c4`, `DAT_0000dd28` |
| charger poll | 1 s | `DAT_000044e4` |
| screen timeout | 5 s, or 15 s | `DAT_0000a710`, `DAT_0000a70c` |
| charge animation step | 90 s | `DAT_0000c36c` |

---

## Dumping every asset

[tools/assetdump.py](tools/assetdump.py) writes the whole asset side of the
firmware to a folder — PNGs to look at, C headers to build with, and a manifest
tying every file back to the flash offset it came from:

```bash
python3 tools/assetdump.py binaries/stock/LT716_V10712_211091429.bin assets
# fonts: 1995 latin glyphs in 2 sheets, 3756 CJK glyphs in 4 sheets
# strings: 7 languages x 78 slots
# graphics: 243 records, 152540 bytes (74% of the asset area), 111 at 1bpp, 132 at 8bpp
#          36 contact sheets for records that share a size
```

```
assets/
├── MANIFEST.csv        kind, offset, length, width, height, file, note
├── fonts/              6 atlas PNGs + font_latin.h, font_cjk.h
├── strings/            lang0.txt … lang6.txt
├── graphics/           243 PNGs, one per record, + sets/ contact sheets
└── headers/            one .h per record, whole record kept intact
```

The output is committed so it is browsable on GitHub and the headers can be used
without running anything. It is still fully derived: the same binary and the same
`reversed/` produce a byte-identical tree, so re-run the tool rather than editing
a file in `assets/`.

### Exact: fonts and strings

The layout is known, so these come out faithful. `font_latin.h` and
`font_cjk.h` are drop-in — raw glyph data plus the indexing macros, and for CJK
the binary search over the codepoint table:

```c
#include "assets/fonts/font_latin.h"
#include "assets/fonts/font_cjk.h"

const unsigned char *g = FONT_LATIN_GLYPH('A');   /* direct index */
int i = font_cjk_index(0x4E00);                    /* binary search */
```

Both compile and run standalone. The atlas sheets confirm the coverage: ASCII,
Latin-1, Latin Extended-A/B, Cyrillic and Greek in the direct-index range, then
3756 CJK ideographs behind the table.

### Graphics: exact, from the record format

The images are **not** raw 1bpp blobs, and they are not laid out end to end.
Each one is a self-describing record whose offsets live in the code, and all of
that came out of the decompilation rather than from staring at bytes:

| Function | What it gave up |
| -------- | --------------- |
| `FUN_000001c8` | the SPI flash read primitive — command 3, 24-bit address |
| `FUN_00004704` | wraps it as `read(ASSET_BASE + offset)`, which fixes **ASSET_BASE at 0x04a828** |
| `FUN_00006a08` | unpacks the record header, and has a second mode that parses a real BMP — for faces uploaded over BLE, since no BMP signature exists in this image |
| `FUN_00007928` | the draw entry point, called with constant offsets — so its 124 call sites *are* the asset index |

The record:

```
u8   width
u8   height
u16  bits per pixel        (1 or 8 here)
u32  palette entry count
n x u16  palette, RGB565 little-endian
rows     ceil(width * bpp / 8) bytes each, top-down, MSB leftmost
```

Sizes confirm it both ways: a 24×11 1bpp record is `8 + 2 + 3*11 = 43` bytes,
matching the spacing between digit-table entries, and the 128×128 face is
`8 + 2*76 + 128*128 = 16544`.

Records are found by **chaining**: a lone plausible header is weak evidence, but
requiring that the *next* record also parses makes it strong. That plus the
code-named offsets accounts for **243 records over 100% of the asset area** —
`0x04a828` to `0x06fc04`, no gaps, and everything above it is erased `0xff`.

> **The mistake worth recording:** reading the `u16` at `+2` as a colour count
> instead of a bit depth. Everything above 1bpp then fails to parse, which
> silently discards the entire 8bpp population — 132 of 243 records, including
> every watch face. An earlier version of this tool guessed widths by
> autocorrelation instead and got the battery icons right by luck; the graphics
> it produced beyond that were wrong.

### There is more than one font

The 12×12 glyph table is only the *text* font. The digits and labels the UI
draws large are image sets, and `assets/graphics/sets/` groups records that share
a size so they are obvious at a glance:

| Set | What it is |
| --- | ---------- |
| `set_16x27_8bpp_x11.png` | the clock font — 0–9 and `:`, 8bpp colour |
| `set_16x14_1bpp_x14.png` | a smaller digit font — 0–9 plus `%`, `-`, `/` |
| `set_16x27_8bpp`, `set_18x27`, `set_24x11`, `set_28x10` … | further digit and label runs per screen |
| `set_56x24_1bpp_x5.png` | the battery icons, five charge levels |

36 sheets in all. So "the fonts" are one glyph table plus a family of
image-based digit sets — which is why the UI can show colour digits the 12×12
font could never produce.

---

## Battery and charging, out of the firmware

The first real payoff from [reversed/](reversed/): the whole battery and charger
path, read out of the decompilation rather than guessed at. Implemented in
[lib/battery/](lib/battery/), verifiable on hardware with
[examples/battery_charge/](examples/battery_charge/).

### The two ADC channels

The stock code selects an ADC input by the SDK's own pin encoding — `port << 8 |
bitmask`, confirmed by `FUN_0000e3d4`, which indexes the port registers with
`param >> 8` and masks with `param & 0xff`:

| Channel | Pin | Scaling in the firmware | mV per count | Of what |
| ------- | --- | ----------------------- | ------------ | ------- |
| `0x102` | PB1 | `avg * 57 / 100 + 71` (`FUN_0000e870`) | 0.57 | the battery, through the PCB's 1:4 divider |
| `0x104` | **PB2** | `(avg * 426 - 990) / 385` (`FUN_0000e9ec`) | 1.107 | the charger rail, through a **1:8** divider |

Both constants are "mV of the thing being measured", not mV at the pin — the
firmware reads each channel with the reference and prescaler that make the
board's own divider come out in real volts.

**PB2 is the charger sense input.** It was on our unknown-pin list, dismissed as
"bulk init only, probably unused" — the sweeps never found it because it is an
analog input that only carries a signal when a charger is attached.

Both reads use the same filter, which is worth copying: 16 samples, sorted, the
four highest and four lowest discarded, the middle eight averaged. On this board
that matters — the motor and the backlight both dent the rail.

### The gauge

`FUN_0000a020` clamps the reading to 3350–4200 mV, then walks a table of seven
`uint16` thresholds at `0x19e28` and takes the level byte at the matching index
from `0x19e40`:

| Battery | Reported |
| ------- | -------- |
| ≥ 4100 mV | 100 % |
| ≥ 3800 mV | 100 % |
| ≥ 3700 mV | 75 % |
| ≥ 3600 mV | 50 % |
| ≥ 3500 mV | 25 % |
| < 3500 mV | 0 % |

So the stock gauge is a five-step display, not a percentage, and the top is flat
on purpose: anything from 3800 mV up reads 100. That is why the watch appears to
sit at full for most of a day and then fall away quickly.

`0x2710` (10000 mV) is a sanity cap on the raw reading, with `0x0d16` (3350 mV)
as the fallback when it trips.

### Charging

`FUN_0000c37c`, reduced to what it does:

```c
adc_select(0x104);                 /* PB2 */
mv = adc_read_mv();
if (((mv - 4400) & 0xffff) > 2100) {   /* outside 4400..6500 */
    charger_present(0);
    PB_OUT &= ~0x20;               /* PB5 low */
} else {
    charger_present(1);
    ...
    PB_OUT |= 0x20;                /* PB5 high, in one branch */
}
```

Charging is a **window**, not a threshold: 4400 mV to 6500 mV on that rail. The
masked subtraction is how the compiler expressed the range test.

The reason it is a window only becomes clear on hardware. The PB2 sense node
follows **whichever of VBAT and VBUS is higher**, so it is never at zero:

| State | Pack | PB2 pin | × 8 = rail | Verdict |
| ----- | ---- | ------- | ---------- | ------- |
| battery, unplugged | 4028 mV | 507 mV | **4056 mV** — the pack (ratio 7.94) | below 4400 → BATTERY ✓ |
| battery, charging | 4200 mV | 652 mV | **5216 mV** — a USB supply | inside window → CHARGING ✓ |
| no battery, on 3V3 | 396 mV | 47 mV | 376 mV | no rail at all ✓ |

Measured on hardware after the fix, and the divider ratio reproduces across
runs: 7.95 on one pack voltage, 7.94 on another.

That is what the lower bound is for: **4400 mV is above a full battery
(4200 mV)**, so a rail reading above it can only be an external supply. Testing
the sense pin against zero would report charging forever.

It also caught a bug on our side. `charger_read_mv()` first returned the *pin*
voltage, so a charging watch reported 653 mV against a window starting at 4400
and the state stayed on BATTERY. The ×8 was missing; the measured 7.95 ratio is
what pinned the divider down.

This also explains **PB5**, the other pin the README used to list as "driven
output, purpose unclear": `FUN_0000c37c` drives it from the charger state.

### How it actually runs

Checked against the call sites, not just the leaf functions:

- The charger check is called from the **10 ms main loop** (`FUN_00004464`) but
  rate-limited to **once per second**. Nothing debounces the result beyond the
  median-of-16 inside the ADC read — one sample per second decides the state.
- **Charger-present and low-battery are two separate flags**, at `+3` and `+0x10`
  of the device state. Charger-present has around a dozen consumers across the
  UI and BLE paths; low-battery only a couple.
- The **low-battery action is gated on NOT charging** (`FUN_0000be6c` tests both),
  so plugging in silences the warning immediately, before the voltage recovers.
  `examples/battery_charge` mirrors that.
- The **charging progress the watch animates is a timer**, not a measurement: a
  counter bumped every 90 s and capped at 100 (`FUN_0000c2ac`). Nothing in it is
  derived from the battery.

Two more constants from the same area:

- **`0xfef` = 4079 mV** — while charging, dropping to or below this re-arms the
  charge path (`FUN_00009fb8(1)`), so it acts as the recharge threshold.
- **`0xd16` = 3350 mV** — low battery. `FUN_0000c2ac` sets the low-battery state
  and pushes a screen below it, and clears it once the voltage rises past it
  again. Same value as the gauge floor, so 0 % and "low" coincide.

`FUN_0000c2ac` also holds the charging *animation*: a counter bumped once every
90 s (`0x055d4a80` µs) and capped at 100. It is a timer, not a measurement — the
progress the watch shows while charging is not derived from the battery at all.

### Trying it

```bash
docker run --rm -v $(pwd):/src -w /src/examples/battery_charge -it tlsr8232-sdk make
python3 tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 write_flash examples/battery_charge/_build/battery_charge.bin
```

Shows the pack voltage, the stock percentage, the charger rail, the raw PB2 pin
voltage behind it, and CHARGING/BATTERY. Expect `CHG` to sit near the pack
voltage when unplugged and jump to about 5200 on the charger — **not** zero
either way. `PIN` is there for calibration: if it disagrees with a multimeter on
PB2, the 1:8 ratio differs on your board and only `CHARGER_DIVIDER` needs
changing.

---

### ⚠️ SWire and running firmware are mutually exclusive

On this board you get debug **or** firmware, never both at once. Two reasons
stack up:

- **SWS is PC7**, a general-purpose pin. The stock firmware is free to claim it,
  and Telink firmware commonly disables the SWire interface after boot.
- **Attaching resets the chip.** `init_soc` starts by pulling RST low, because
  the ET24 package has no RST pin and the chip has to be caught in SWire mode at
  power-on. So connecting restarts whatever was running.

`watch_gpio --no-reset` attaches without the reset or the CPU halt, which is the
only way to look at a chip mid-run. If reads fail, or come back and never change,
that is the answer rather than a misconfiguration — and the live-watch approach
below is off the table for this hardware.

### Finding a pin without SWire

The app can trigger the vibration on command, and that is enough on its own — no
debugger required.

1. **Continuity, power off.** Deterministic and takes a minute. Find the motor
   and the small transistor next to it (SOT-23/SOT-323, usually with a flyback
   diode across the motor pads). Beep from the transistor's base/gate — or the
   far end of its series resistor — to each candidate package pin: 3 (PA3),
   4 (PA4), 9 (PA5), 12 (PB2), 23 (PC4). Exactly one reads as a short.
2. **Voltage during a buzz.** If the driver is buried, put a DMM on DC volts
   between GND and each candidate pin in turn and hit vibrate in the app. The
   motor pin swings to ~3 V. A single buzz is brief for a DMM, so use repeated
   buzzes (an alarm or an incoming-call notification) or the meter's max-hold.
3. **Static analysis.** `ghidra_project/GpioPinMap.java` decodes stores to the
   GPIO registers, and it now has a trustworthy image to run against. Every bit
   the firmware drives, minus the ones already mapped, is a short candidate list.

### Watching the stock firmware instead — not viable here, see above

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
│   ├── battery_charge/     # Stock battery gauge + charger detection on PB2
│   ├── pin_probe/          # Unknown-pin input census + touch key hunt
│   ├── vib_hunt/           # Unknown-pin output sweep (vibrator motor hunt)
│   ├── vib_sweep/          # Vibrator hunt pass 2: AC sweep + pin pairs
│   ├── vib_probe/          # Vibrator hunt by battery sag (no buzz needed)
│   ├── touch_key/          # PC2 touch key characterisation + logic trace
│   ├── touch_demo/         # Touch driver demo: tap / double tap / long press
│   ├── vibrate/            # Vibrator motor patterns on PA5
│   ├── vib_pa5/            # PA5 frequency sweep, scored by battery sag
│   ├── uart/               # UART debug output
│   ├── ble_adv/            # BLE advertising
│   └── blink/              # Backlight blink
├── lib/                    # Reusable libraries
│   ├── display/            # ST7735 display driver
│   ├── fonts/              # Bitmap fonts (Adafruit GFX format)
│   ├── battery/            # Battery gauge + charger sense, from the firmware
│   ├── pinscan/            # Unknown-pin candidate list + probe/drive helpers
│   ├── touch/              # PC2 touch key driver (debounce, tap/long/double)
│   ├── vibrate/            # PA5 vibrator motor driver
│   └── uart/               # UART helper
├── ghidra_project/         # Ghidra database + analysis scripts (see below)
│   └── decompiled/         # Whole stock image decompiled to C, 1039 functions
├── reversed/               # That decompilation as a buildable C project
├── sdk/                    # Docker-based build environment
└── tools/
    ├── fwtool.py           # Stock firmware explorer: map, glyphs, strings, bitmaps
    ├── decomp2proj.py      # Ghidra decompilation → buildable project in reversed/
    ├── assetdump.py        # Stock assets → PNGs + C headers + manifest
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
