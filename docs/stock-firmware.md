# Stock Firmware

Layout, decompilation and behaviour of the stock firmware, and the dumps it
was read from.

## Stock Firmware Structure

Reverse engineered from `binaries/stock/LT716_V10712_211091429.bin` with
[tools/fwtool.py](../tools/fwtool.py). Every finding below was verified by decoding
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
[ghidra_project/decompiled/LT716_V10712_211091429.c](../ghidra_project/decompiled/LT716_V10712_211091429.c)
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

[reversed/](../reversed/) takes that file the rest of the way: generated headers, a
Makefile, and C the compiler accepts.

```bash
docker run --rm -v $(pwd):/src -w /src/reversed ghcr.io/e-labinnovations/tlsr8232-sdk make
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

**It compiles; it will never run** — and [reversed/README.md](../reversed/README.md)
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
([lib/touch](../lib/touch/)) for exactly that reason. Note there is **no double-tap
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

## Stock firmware dumps

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
