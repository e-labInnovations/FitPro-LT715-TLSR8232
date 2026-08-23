#!/usr/bin/env python3
"""Extract every asset from the stock LT716 firmware into a folder.

Writes PNGs to look at, C headers to build with, and a manifest tying each file
back to the flash offset it came from:

    assets/
      MANIFEST.csv          kind, offset, length, width, height, file
      fonts/                glyph atlases (PNG) + font_latin.h, font_cjk.h
      graphics/             one PNG per detected image
      headers/              one .h per detected image, 1bpp rows
      strings/              the UI string table, one file per language

Fonts and strings come out exactly: their layout is known (see the README
section "Stock Firmware Structure"), so those files are faithful.

The graphics are different, and worth being clear about. Nothing in the image
records where one picture ends and the next begins - the firmware passes offsets
and dimensions from code. So this segments the region on runs of blank bytes and
estimates each width by autocorrelation, which recovers most assets but will
merge neighbours that touch and split any image containing a wide blank band.
Treat graphics/ as a contact sheet: find the one you want, then re-render it
exactly with `fwtool.py bitmap --off ... --width ... --rows ...`.

Usage:
    python3 tools/assetdump.py binaries/stock/LT716_V10712_211091429.bin assets
    python3 tools/assetdump.py FW assets --no-headers      # PNGs only
    python3 tools/assetdump.py FW assets --gap 32 --scale 2
"""

import argparse
import os
import struct
import sys
import zlib

# Layout constants, same as tools/fwtool.py
FONT_BASE  = 0x028fff      # addr(codepoint) = FONT_BASE + cp * 18
FONT_LAST  = 0x07cb        # last codepoint with a direct slot
CJK_TABLE  = 0x031c58      # 3756 x uint16, sorted codepoints
CJK_COUNT  = 3756
CJK_GLYPHS = 0x0339b0
STR_MAGIC  = 0x0441c8
STR_BASE   = 0x044200
STR_SLOT   = 0x30
STR_STRIDE = 0xea0
STR_SLOTS  = STR_STRIDE // STR_SLOT
STR_LANGS  = 7
GLYPH_SIZE = 18
GFX_START  = 0x04a860      # 1bpp graphics, battery icons first
GFX_END    = 0x06fc04

INK = (255, 255, 255)
BG  = (20, 20, 20)


# ------------------------------------------------------------------ PNG output

def png(path, w, h, rows):
    def chunk(t, data):
        return (struct.pack('>I', len(data)) + t + data
                + struct.pack('>I', zlib.crc32(t + data) & 0xffffffff))
    raw = b''.join(b'\x00' + bytes(r) for r in rows)
    with open(path, 'wb') as fh:
        fh.write(b'\x89PNG\r\n\x1a\n'
                 + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                 + chunk(b'IDAT', zlib.compress(raw, 9))
                 + chunk(b'IEND', b''))


def bits_to_rows(bits, scale):
    """bits: list of rows, each a list of 0/1. Returns RGB rows for png()."""
    out = []
    for row in bits:
        line = []
        for v in row:
            line += list(INK if v else BG) * scale
        for _ in range(scale):
            out.append(list(line))
    return out


# ----------------------------------------------------------------------- fonts

def glyph_bits(d, off):
    """A glyph is 12x12 in 18 bytes: 12 bytes of the left 8 columns, one bit per
    pixel, then 6 bytes holding the right 4 columns two rows at a time."""
    rows = []
    for r in range(12):
        left = d[off + r]
        b = d[off + 12 + r // 2]
        nib = (b >> 4) if r % 2 == 0 else (b & 0x0f)
        rows.append([(left >> (7 - i)) & 1 for i in range(8)]
                    + [(nib >> (3 - i)) & 1 for i in range(4)])
    return rows


def atlas(d, out_dir, name, addrs, labels, cols, scale, per_sheet, manifest):
    """Grid of glyphs, split across sheets so no single PNG gets unwieldy."""
    sheets = 0
    for base in range(0, len(addrs), per_sheet):
        part = addrs[base:base + per_sheet]
        grid_rows = (len(part) + cols - 1) // cols
        bits = [[0] * (cols * 12) for _ in range(grid_rows * 12)]
        for i, off in enumerate(part):
            gx, gy = (i % cols) * 12, (i // cols) * 12
            for y, row in enumerate(glyph_bits(d, off)):
                for x, v in enumerate(row):
                    bits[gy + y][gx + x] = v
        fn = "%s_%02d.png" % (name, sheets)
        rows = bits_to_rows(bits, scale)
        png(os.path.join(out_dir, fn), len(rows[0]) // 3, len(rows), rows)
        manifest.append(("font-atlas", addrs[base], len(part) * GLYPH_SIZE,
                         cols * 12, grid_rows * 12,
                         "fonts/" + fn,
                         "%s..%s" % (labels[base], labels[min(base + per_sheet,
                                                             len(labels)) - 1])))
        sheets += 1
    return sheets


def c_array(name, data, per_line=16):
    out = ["static const unsigned char %s[%d] = {" % (name, len(data))]
    for i in range(0, len(data), per_line):
        out.append("    " + ", ".join("0x%02x" % b for b in data[i:i + per_line]) + ",")
    out.append("};")
    return "\n".join(out)


def dump_fonts(d, root, args, manifest):
    fdir = os.path.join(root, "fonts")
    os.makedirs(fdir, exist_ok=True)

    # Latin and symbol range: a direct slot per codepoint
    latin = [FONT_BASE + cp * GLYPH_SIZE for cp in range(1, FONT_LAST + 1)]
    labels = ["U+%04X" % cp for cp in range(1, FONT_LAST + 1)]
    n = atlas(d, fdir, "latin_atlas", latin, labels,
              args.cols, args.scale, args.per_sheet, manifest)

    # CJK: a sorted codepoint table, then glyphs in the same order
    cps = [struct.unpack_from('<H', d, CJK_TABLE + 2 * i)[0] for i in range(CJK_COUNT)]
    cjk = [CJK_GLYPHS + i * GLYPH_SIZE for i in range(CJK_COUNT)]
    m = atlas(d, fdir, "cjk_atlas", cjk, ["U+%04X" % c for c in cps],
              args.cols, args.scale, args.per_sheet, manifest)
    print("fonts: %d latin glyphs in %d sheets, %d CJK glyphs in %d sheets"
          % (len(latin), n, len(cjk), m))

    if args.headers:
        blob = d[FONT_BASE + GLYPH_SIZE:FONT_BASE + (FONT_LAST + 1) * GLYPH_SIZE]
        with open(os.path.join(fdir, "font_latin.h"), "w") as fh:
            fh.write('''/* Latin and symbol glyphs from the stock LT716 firmware.
 *
 * Extracted from flash 0x%06x by tools/assetdump.py. Codepoints U+0001..U+%04X,
 * %d bytes each, indexed directly:
 *
 *     const unsigned char *g = FONT_LATIN_GLYPH(cp);
 *
 * Each glyph is 12x12 split across two planes. Bytes 0..11 are the left 8
 * columns, one byte per row, most significant bit leftmost. Bytes 12..17 hold
 * the right 4 columns packed two rows to a byte, high nibble first:
 *
 *     pixel(x, y) = x < 8 ? (g[y] >> (7 - x)) & 1
 *                         : (g[12 + y/2] >> (y %% 2 ? 3 - (x - 8)
 *                                                   : 7 - (x - 8))) & 1
 *
 * Latin glyphs leave the right plane zero, which is why a naive 8x18 read of
 * this data looks correct until you reach a CJK glyph.
 */

#ifndef FONT_LATIN_H
#define FONT_LATIN_H

#define FONT_LATIN_FIRST   0x0001
#define FONT_LATIN_LAST    0x%04X
#define FONT_GLYPH_BYTES   %d
#define FONT_GLYPH_W       12
#define FONT_GLYPH_H       12

#define FONT_LATIN_GLYPH(cp) \\
    (FONT_LATIN_DATA + ((cp) - FONT_LATIN_FIRST) * FONT_GLYPH_BYTES)

%s

#define FONT_LATIN_DATA font_latin_data

#endif /* FONT_LATIN_H */
''' % (FONT_BASE + GLYPH_SIZE, FONT_LAST, GLYPH_SIZE, FONT_LAST, GLYPH_SIZE,
       c_array("font_latin_data", blob)))
        manifest.append(("font-header", FONT_BASE + GLYPH_SIZE, len(blob),
                         12, 12, "fonts/font_latin.h",
                         "U+0001..U+%04X" % FONT_LAST))

        idx = d[CJK_TABLE:CJK_TABLE + 2 * CJK_COUNT]
        gly = d[CJK_GLYPHS:CJK_GLYPHS + CJK_COUNT * GLYPH_SIZE]
        with open(os.path.join(fdir, "font_cjk.h"), "w") as fh:
            fh.write('''/* CJK glyphs from the stock LT716 firmware.
 *
 * Extracted by tools/assetdump.py: a sorted table of %d codepoints from flash
 * 0x%06x, then the glyphs in the same order from 0x%06x. Same 12x12 two-plane
 * format as font_latin.h, so binary search the index and multiply:
 *
 *     int i = font_cjk_index(cp);
 *     const unsigned char *g = i < 0 ? 0 : FONT_CJK_DATA + i * FONT_GLYPH_BYTES;
 */

#ifndef FONT_CJK_H
#define FONT_CJK_H

#define FONT_CJK_COUNT %d

%s

%s

#define FONT_CJK_INDEX font_cjk_index_data
#define FONT_CJK_DATA  font_cjk_glyph_data

/* Binary search, the way the firmware itself must do it. */
static inline int font_cjk_index(unsigned short cp) {
    int lo = 0, hi = FONT_CJK_COUNT - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        unsigned short v = (unsigned short)(FONT_CJK_INDEX[mid * 2]
                            | (FONT_CJK_INDEX[mid * 2 + 1] << 8));
        if (v == cp) return mid;
        if (v < cp)  lo = mid + 1;
        else         hi = mid - 1;
    }
    return -1;
}

#endif /* FONT_CJK_H */
''' % (CJK_COUNT, CJK_TABLE, CJK_GLYPHS, CJK_COUNT,
       c_array("font_cjk_index_data", idx),
       c_array("font_cjk_glyph_data", gly)))
        manifest.append(("font-header", CJK_GLYPHS, len(gly), 12, 12,
                         "fonts/font_cjk.h", "%d CJK glyphs" % CJK_COUNT))


# --------------------------------------------------------------------- strings

def slot_text(d, off):
    out = []
    for i in range(0, STR_SLOT - 1, 2):
        v = d[off + i] | (d[off + i + 1] << 8)
        if v == 0:
            break
        out.append(chr(v))
    return ''.join(out)


def dump_strings(d, root, manifest):
    sdir = os.path.join(root, "strings")
    os.makedirs(sdir, exist_ok=True)
    magic = struct.unpack_from('<H', d, STR_MAGIC)[0]
    for lang in range(STR_LANGS):
        base = STR_BASE + lang * STR_STRIDE
        lines = []
        for slot in range(STR_SLOTS):
            t = slot_text(d, base + slot * STR_SLOT)
            if t:
                lines.append("%3d  %s" % (slot, t))
        fn = "lang%d.txt" % lang
        with open(os.path.join(sdir, fn), "w") as fh:
            fh.write("# language %d, table at 0x%06x, magic 0x%04x\n"
                     % (lang, base, magic))
            fh.write("\n".join(lines) + "\n")
        manifest.append(("strings", base, STR_STRIDE, 0, 0,
                         "strings/" + fn, "%d slots used" % len(lines)))
    print("strings: %d languages x %d slots" % (STR_LANGS, STR_SLOTS))


# -------------------------------------------------------------------- graphics

def find_segments(d, args):
    """Split the graphics region where the row stride changes.

    Splitting on blank runs alone does not work: consecutive images butt right
    up against each other with no separator, so a run of blanks finds some
    boundaries and misses many. What does change at a boundary is the row
    stride, since the images are different widths.

    So: estimate a local width for every block of the region, then merge runs of
    blocks that agree. Blocks that are entirely blank act as separators. This is
    still inference, not information from the firmware - two neighbours that
    happen to share a width stay merged.
    """
    blocks = []
    pos = GFX_START
    while pos < GFX_END:
        end = min(pos + args.block, GFX_END)
        chunk = d[pos:end]
        if not any(chunk):
            blocks.append((pos, end, None))
        else:
            blocks.append((pos, end, guess_width(d, pos, end, args.max_width)))
        pos = end

    segs, cur = [], None
    for start, end, w in blocks:
        if w is None:
            if cur:
                segs.append(cur)
                cur = None
            continue
        if cur and cur[2] == w:
            cur = (cur[0], end, w)
        else:
            if cur:
                segs.append(cur)
            cur = (start, end, w)
    if cur:
        segs.append(cur)

    out = []
    for start, end, w in segs:
        # trim trailing blanks, then snap to a whole number of rows measured
        # from the region start so rows do not come out sheared
        while end > start and d[end - 1] == 0:
            end -= 1
        phase = (start - GFX_START) % w
        start -= phase
        if end - start >= max(args.min_len, 2 * w):
            out.append((start, end, w))
    return out


def guess_width(d, start, end, max_w, window=2048):
    """Autocorrelation over the segment: rows of a picture resemble the rows
    above and below them, so the true row stride scores highest.

    Two details matter. Most of this data is blank, so scoring byte equality
    outright rewards every short lag for matching zero against zero - pairs
    where both bytes are blank are skipped. And the true width's multiples
    score nearly as well, so once a winner is found its sub-harmonics are
    checked and the smallest near-equal one wins.

    Scored over a window from the segment start rather than the whole segment:
    if the segmentation merged two images, the first one still dominates.
    """
    win = d[start:min(end, start + window)]
    n = len(win)
    scored = []
    for lag in range(2, min(max_w, n // 3) + 1):
        tot = hit = 0
        for i in range(n - lag):
            a, b = win[i], win[i + lag]
            if a or b:
                tot += 1
                hit += (a == b)
        if tot:
            scored.append((hit / tot, lag))
    if not scored:
        return max(1, n)

    best_score, best = max(scored)
    by_lag = dict((lag, sc) for sc, lag in scored)
    for div in range(2, best + 1):
        if best % div == 0:
            cand = best // div
            if cand >= 2 and by_lag.get(cand, 0) >= best_score * 0.9:
                best = cand
                break
    return best


def dump_graphics(d, root, args, manifest):
    gdir = os.path.join(root, "graphics")
    hdir = os.path.join(root, "headers")
    os.makedirs(gdir, exist_ok=True)
    if args.headers:
        os.makedirs(hdir, exist_ok=True)

    segs = find_segments(d, args)
    if args.limit:
        segs = segs[:args.limit]
    print("graphics: %d segments between 0x%06x and 0x%06x"
          % (len(segs), GFX_START, GFX_END))

    for start, end, wbytes in segs:
        nrows = (end - start) // wbytes
        if nrows < 2:
            continue
        bits = []
        for y in range(nrows):
            row = []
            for xb in range(wbytes):
                b = d[start + y * wbytes + xb]
                row += [(b >> (7 - i)) & 1 for i in range(8)]
            bits.append(row)

        # Some assets store ink as 1, others as 0 - the battery icons come out
        # as black shapes on white if taken literally. Anything more than half
        # set is treated as inverted so the PNG reads as ink on dark. The
        # header keeps the original bytes either way.
        ink = sum(sum(r) for r in bits)
        inverted = ink * 2 > nrows * wbytes * 8
        if inverted:
            bits = [[1 - v for v in r] for r in bits]

        stem = "gfx_%06x_w%d" % (start, wbytes * 8)
        rows = bits_to_rows(bits, args.scale)
        png(os.path.join(gdir, stem + ".png"), len(rows[0]) // 3, len(rows), rows)
        manifest.append(("graphic", start, end - start, wbytes * 8, nrows,
                         "graphics/" + stem + ".png",
                         "width estimated" + (", shown inverted" if inverted else "")))

        if args.headers:
            with open(os.path.join(hdir, stem + ".h"), "w") as fh:
                fh.write('''/* 1bpp image from the stock LT716 firmware at flash 0x%06x.
 *
 * Extracted by tools/assetdump.py. %d bytes per row, %d rows, so %d x %d pixels
 * with the most significant bit of each byte leftmost.
 *
 * The width was ESTIMATED by autocorrelation, not read from the firmware -
 * nothing in the image records it. If this renders skewed, the real width is
 * probably a divisor or multiple of this one; check with
 *   python3 tools/fwtool.py FW width 0x%06x
 */

#ifndef %s_H
#define %s_H

#define %s_W      %d
#define %s_H_PX   %d
#define %s_STRIDE %d

%s

#endif
''' % (start, wbytes, nrows, wbytes * 8, nrows, start,
       stem.upper(), stem.upper(), stem.upper(), wbytes * 8,
       stem.upper(), nrows, stem.upper(), wbytes,
       c_array(stem + "_data", d[start:start + wbytes * nrows])))
            manifest.append(("graphic-header", start, wbytes * nrows,
                             wbytes * 8, nrows, "headers/" + stem + ".h",
                             "width estimated"))


# ------------------------------------------------------------------------ main

README = '''# Stock firmware assets

Extracted from `%s` by `tools/assetdump.py`. Everything here is derived - re-run
the tool rather than editing these files.

`MANIFEST.csv` lists every file with the flash offset it came from.

## fonts/

Exact. The layout is known: glyphs are 12x12 in 18 bytes, split into two planes
(bytes 0..11 the left 8 columns one row per byte, bytes 12..17 the right 4
columns packed two rows per byte). Latin codepoints index directly from
0x%06x; CJK goes through the sorted codepoint table at 0x%06x.

`font_latin.h` and `font_cjk.h` are drop-in: raw data plus the indexing macros
and, for CJK, the binary search.

## strings/

Exact. Seven languages, %d slots each, UTF-16 code units, table at 0x%06x.

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
'''


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("firmware")
    ap.add_argument("outdir")
    ap.add_argument("--scale", type=int, default=2, help="PNG pixel scale (default 2)")
    ap.add_argument("--cols", type=int, default=32, help="glyphs per atlas row")
    ap.add_argument("--per-sheet", type=int, default=1024, help="glyphs per atlas PNG")
    ap.add_argument("--block", type=int, default=256,
                    help="bytes per width probe when segmenting (default 256)")
    ap.add_argument("--min-len", type=int, default=32,
                    help="ignore segments smaller than this (default 32)")
    ap.add_argument("--max-width", type=int, default=32,
                    help="largest row stride to consider, in bytes (default 32)")
    ap.add_argument("--limit", type=int, default=0, help="stop after N graphics")
    ap.add_argument("--no-headers", dest="headers", action="store_false",
                    help="PNGs and text only, skip the C headers")
    ap.add_argument("--skip-graphics", action="store_true")
    args = ap.parse_args()

    d = open(args.firmware, 'rb').read()
    if len(d) < GFX_END:
        sys.exit("image is %d bytes, too short to hold the asset region" % len(d))

    root = args.outdir
    os.makedirs(root, exist_ok=True)
    manifest = []

    dump_fonts(d, root, args, manifest)
    dump_strings(d, root, manifest)
    if not args.skip_graphics:
        dump_graphics(d, root, args, manifest)

    with open(os.path.join(root, "MANIFEST.csv"), "w") as fh:
        fh.write("kind,offset,length,width_px,height_px,file,note\n")
        for kind, off, ln, w, h, path, note in manifest:
            fh.write("%s,0x%06x,%d,%d,%d,%s,%s\n" % (kind, off, ln, w, h, path, note))

    with open(os.path.join(root, "README.md"), "w") as fh:
        fh.write(README % (os.path.basename(args.firmware), FONT_BASE, CJK_TABLE,
                           STR_SLOTS, STR_BASE))

    print("wrote %d entries to %s/MANIFEST.csv" % (len(manifest), root))


if __name__ == "__main__":
    main()
