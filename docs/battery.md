# Battery and Charging

## Battery and charging, out of the firmware

The first real payoff from [reversed/](../reversed/): the whole battery and charger
path, read out of the decompilation rather than guessed at. Implemented in
[lib/battery/](../lib/battery/), verifiable on hardware with
[examples/battery_charge/](../examples/battery_charge/).

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
docker run --rm -v $(pwd):/src -w /src/examples/battery_charge ghcr.io/e-labinnovations/tlsr8232-sdk make
python3 tools/tlsr82-debugger-client/tlsr82-debugger-client.py \
  --serial-port /dev/ttyACM0 write_flash examples/battery_charge/_build/battery_charge.bin
```

Shows the pack voltage, the stock percentage, the charger rail, the raw PB2 pin
voltage behind it, and CHARGING/BATTERY. Expect `CHG` to sit near the pack
voltage when unplugged and jump to about 5200 on the charger — **not** zero
either way. `PIN` is there for calibration: if it disagrees with a multimeter on
PB2, the 1:8 ratio differs on your board and only `CHARGER_DIVIDER` needs
changing.
