# Stock Assets

## Dumping every asset

[tools/assetdump.py](../tools/assetdump.py) writes the whole asset side of the
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
