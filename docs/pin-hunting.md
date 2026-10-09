# Finding the Pins

How each unknown pin was found: probe firmware, sweeps, and finally reading
them out of the stock firmware.

## Finding Unknown Pins

Two firmware examples hunt the remaining six pins (PA3, PA4, PA5, PB2, PC2, PC4).
Both report on the watch's own screen, so no probe wiring is needed; UART logging
on PB4/PB5 is optional and controlled by `ENABLE_UART` at the top of `main.c`.
The shared candidate list and the probe helpers live in [lib/pinscan/](../lib/pinscan/).

### 1. `pin_probe` — touch key and other inputs

```bash
docker run --rm -v $(pwd):/src -w /src/examples/pin_probe ghcr.io/e-labinnovations/tlsr8232-sdk make
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
docker run --rm -v $(pwd):/src -w /src/examples/vib_hunt ghcr.io/e-labinnovations/tlsr8232-sdk make
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
docker run --rm -v $(pwd):/src -w /src/examples/touch_key ghcr.io/e-labinnovations/tlsr8232-sdk make
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
`last` grows with the hold. That became [lib/touch/](../lib/touch/).

### Using the touch key

[lib/touch/touch.h](../lib/touch/touch.h) turns PC2 into a UI button: 24 ms
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
docker run --rm -v $(pwd):/src -w /src/examples/touch_demo ghcr.io/e-labinnovations/tlsr8232-sdk make
```

### 4. `vib_sweep` — vibrator motor, second pass

```bash
docker run --rm -v $(pwd):/src -w /src/examples/vib_sweep ghcr.io/e-labinnovations/tlsr8232-sdk make
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
docker run --rm -v $(pwd):/src -w /src/examples/vib_probe ghcr.io/e-labinnovations/tlsr8232-sdk make
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

---

## Reading the pins out of the firmware

The pin sweeps never found the motor. Decompiling the stock firmware did, in one
sitting — and it turns out **Ghidra 12 ships a `Telink_TC32` processor module**,
so no community plugin is needed. Disassembly of the main application yields
34 729 instructions, and analysis of the full image finds 1040 functions — the
decompiler produces clean C for 1039 of them.

The scripts are in [ghidra_project/](../ghidra_project/):

| Script | What it does |
| ------ | ------------ |
| `SetupBlocks.java` | run as `-preScript` on import: adds the register and SRAM blocks, marks both KNLT entry points |
| `GpioMapAll.java` | one table of every GPIO access in the image — function → registers → pin bits |
| `GpioRefs.java` | same idea for one port, but prints the disassembly window and C for each hit |
| `Pa5Sites.java` | disassembly around every PA access, flagging bit-5 masks — how the motor pin was pinned down |
| `DecompAll.java` | decompiles every function to one C file (see [stock-firmware.md](stock-firmware.md#decompiling-the-whole-image-to-c)) |
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

## Debugging a running watch

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
another one, and the dumps in this repo are not a substitute (see [Stock firmware dumps](stock-firmware.md#stock-firmware-dumps)):

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
first, for the reason in [Stock firmware dumps](stock-firmware.md#stock-firmware-dumps).
