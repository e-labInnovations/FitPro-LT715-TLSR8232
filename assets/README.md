# Stock firmware assets

Extracted from `LT716_V10712_211091429.bin` by `tools/assetdump.py`. Everything here is derived - re-run
the tool rather than editing these files.

`MANIFEST.csv` lists every file with the flash offset it came from.

## fonts/

Exact. The layout is known: glyphs are 12x12 in 18 bytes, split into two planes
(bytes 0..11 the left 8 columns one row per byte, bytes 12..17 the right 4
columns packed two rows per byte). Latin codepoints index directly from
0x028fff; CJK goes through the sorted codepoint table at 0x031c58.

`font_latin.h` and `font_cjk.h` are drop-in: raw data plus the indexing macros
and, for CJK, the binary search.

## strings/

Exact. Seven languages, 78 slots each, UTF-16 code units, table at 0x044200.

## graphics/ and headers/

**Estimated, not exact.** Nothing in the firmware records where one image ends
and the next begins, or how wide any of them are - the code passes offsets and
dimensions as constants. These files come from segmenting the region on blank
runs and estimating each width by autocorrelation.

So expect two failure modes: images that touch get merged, and an image with a
wide blank band gets split. Use this as a contact sheet - find what you want,
then render it exactly:

```bash
python3 tools/fwtool.py FW width  0x04a860          # check the stride
python3 tools/fwtool.py FW bitmap out.png --off 0x04a860 --width 7 --rows 180
```

The battery icons are the first thing in the region, at 0x04a860, 7 bytes (56 px)
wide.
