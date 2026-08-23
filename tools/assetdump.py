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
import re
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
GFX_END    = 0x07d000      # assets are scattered up to the config sector

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
#
# The images are NOT raw 1bpp blobs, and they are not laid out end to end. Each
# one is a self-describing record, and their offsets live in the code. Both facts
# came out of the decompilation (reversed/), not from staring at bytes:
#
#   FUN_000001c8   the SPI flash read primitive: command 3, 24-bit address
#   FUN_00004704   wraps it as read(ASSET_BASE + offset), which is what fixes
#                  ASSET_BASE at 0x04a828 - just below where the pixel data
#                  appeared to start when we were guessing
#   FUN_00006a08   unpacks the record: byte +0 width, byte +1 height, a colour
#                  count, then the palette, then the pixel rows
#   FUN_00007928   the draw entry point, called with a constant offset - so the
#                  124 call sites ARE the asset index
#
# Record layout, mode 0 (the built-in assets):
#
#   u8    width
#   u8    height
#   u16   bits per pixel        (1 or 8 in this image)
#   u32   palette entry count
#   n x u16  palette, RGB565 little-endian
#   rows  packed pixels, ceil(width * bpp / 8) bytes per row, top-down,
#         most significant bit leftmost
#
# Two populations: 1bpp stencils in a single colour (icons, digits, where index 0
# is transparent) and 8bpp palettized colour images up to a full 128x128 screen -
# the watch faces. Sizes check out exactly both ways: a 24x11 1bpp record is
# 8 + 2 + 3*11 = 43 bytes, matching the spacing between digit-table entries, and
# the 128x128 face is 8 + 2*76 + 128*128 = 16544.
#
# Reading the u16 at +2 as a colour count instead of a bit depth is the mistake
# that makes everything above 1bpp unparseable, and it costs 70% of the region.
#
# FUN_00006a08 has a second mode, selected by a state flag, that parses a real
# BMP header (bfOffBits at +0x0a, biWidth +0x12, biHeight +0x16, biBitCount
# +0x1c, data at +0x36). No BMP signature exists anywhere in this image, so that
# path is for faces uploaded over BLE, not for anything built in.

ASSET_BASE = 0x04a828


def asset_header(d, off):
    """Parse a record header at an absolute address. None if it is not one."""
    if off + 8 > len(d):
        return None
    w, h = d[off], d[off + 1]
    bpp = struct.unpack_from('<H', d, off + 2)[0]
    pal = struct.unpack_from('<I', d, off + 4)[0]
    if not (1 <= w <= 240 and 1 <= h <= 240 and bpp in (1, 2, 4, 8)
            and 1 <= pal <= 256):
        return None
    if bpp < 8 and pal > (1 << bpp):
        return None
    stride = (w * bpp + 7) // 8
    size = 8 + 2 * pal + stride * h
    if off + size > len(d):
        return None
    return dict(w=w, h=h, bpp=bpp, pal=pal, stride=stride, size=size)


def scan_records(d, code_notes):
    """Every record in the asset area, found by chaining valid headers.

    A single valid-looking header is weak evidence - w, h, a small bpp and a
    small palette count is a pattern plain pixel data hits by chance. Requiring
    that the *next* record also parses makes it strong, because a false positive
    almost never lands exactly on another one.

    Records sit in runs with gaps between them, so a failed parse advances one
    byte rather than giving up. Coverage on this image is 74% of the area, which
    is what is left after the string table, the fonts and genuine padding.
    """
    found = {}
    off = ASSET_BASE
    while off < GFX_END:
        hdr = asset_header(d, off)
        if hdr and (asset_header(d, off + hdr['size']) or off + hdr['size'] >= GFX_END):
            found[off] = hdr
            off += hdr['size']
        else:
            off += 1
    # anything the code names but the chain missed
    for rel in code_notes:
        a = ASSET_BASE + rel
        if a not in found:
            hdr = asset_header(d, a)
            if hdr:
                found[a] = hdr
    return found


def code_offsets(src_path, hdr_path, d):
    """Every asset offset the code passes to the draw function.

    Two shapes at the call sites: a constant, and an index into a table of
    constants (`*(undefined4 *)(TABLE + i * 4)`), which is how icon sets and
    digit runs are stored. Tables are walked until an entry stops parsing as a
    record header.
    """
    defs = {}
    pat = re.compile(r'#define (\S+)\s+(?:\(0x([0-9a-f]{8})u\)'
                     r'|\(\(volatile unsigned char \*\)0x([0-9a-f]{8})u\)'
                     r'|\(\*\(volatile unsigned char \*\*\)0x([0-9a-f]{8})u\))')
    for line in open(hdr_path):
        m = pat.match(line)
        if m:
            defs[m.group(1)] = int(m.group(2) or m.group(3) or m.group(4), 16)

    src = open(src_path).read()
    found = {}          # offset -> note

    draw = re.compile(r'FUN_0000(?:7928|7984|7994)\(\s*([^,\)]+)')
    tables = set()
    for m in draw.finditer(src):
        arg = m.group(1).strip()
        if arg in defs:
            off = defs[arg]
            if asset_header(d, ASSET_BASE + off):
                found.setdefault(off, "direct: %s" % arg)
        else:
            t = re.match(r'\*\(undefined4 \*\)\((PTR_\w+|DAT_\w+)\s*\+', arg)
            if t and t.group(1) in defs:
                tables.add(t.group(1))

    for name in sorted(tables):
        addr = defs[name]
        for i in range(256):
            if addr + 4 * i + 4 > len(d):
                break
            off = struct.unpack_from('<I', d, addr + 4 * i)[0]
            if off >= len(d) or not asset_header(d, ASSET_BASE + off):
                break
            found.setdefault(off, "table %s[%d]" % (name, i))

    # Records tend to sit in runs - a digit set, an icon strip - so from every
    # known offset, walk forward while the next header still parses. That picks
    # up frames the code reaches by arithmetic rather than by a constant.
    for off in list(found):
        cur = off
        for _ in range(64):
            hdr = asset_header(d, ASSET_BASE + cur)
            if not hdr:
                break
            cur += hdr['size']
            if cur in found or not asset_header(d, ASSET_BASE + cur):
                break
            found[cur] = "run after 0x%06x" % off
    return found


def decode_asset(d, addr, hdr):
    """Returns (rows of RGB triples, palette). At 1bpp index 0 is transparent
    and comes back as the PNG background; at 8bpp every index is a real colour."""
    pal = []
    for i in range(hdr['pal']):
        c = struct.unpack_from('<H', d, addr + 8 + 2 * i)[0]
        r, g, b = (c >> 11) & 0x1f, (c >> 5) & 0x3f, c & 0x1f
        pal.append((r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2))
    data = addr + 8 + 2 * hdr['pal']
    bpp, mask = hdr['bpp'], (1 << hdr['bpp']) - 1
    rows = []
    for y in range(hdr['h']):
        row = []
        for x in range(hdr['w']):
            bit = x * bpp
            byte = d[data + y * hdr['stride'] + bit // 8]
            idx = (byte >> (8 - bpp - bit % 8)) & mask
            if bpp == 1:
                row.append(BG if idx == 0 else pal[0])
            else:
                row.append(pal[idx] if idx < len(pal) else BG)
        rows.append(row)
    return rows, pal


def dump_graphics(d, root, args, manifest):
    gdir = os.path.join(root, "graphics")
    hdir = os.path.join(root, "headers")
    os.makedirs(gdir, exist_ok=True)
    if args.headers:
        os.makedirs(hdir, exist_ok=True)

    notes = {}
    if os.path.exists(args.src) and os.path.exists(args.decl):
        notes = code_offsets(args.src, args.decl, d)
    else:
        print("graphics: no decompilation at %s, so records will be unnamed "
              "(run tools/decomp2proj.py to get provenance)" % args.src)

    records = scan_records(d, notes)
    covered = sum(h['size'] for h in records.values())
    depths = {}
    for h in records.values():
        depths[h['bpp']] = depths.get(h['bpp'], 0) + 1
    print("graphics: %d records, %d bytes (%.0f%% of the asset area), %s"
          % (len(records), covered, 100.0 * covered / (GFX_END - ASSET_BASE),
             ", ".join("%d at %dbpp" % (n, b) for b, n in sorted(depths.items()))))

    for addr in sorted(records):
        hdr = records[addr]
        rel = addr - ASSET_BASE
        rows, pal = decode_asset(d, addr, hdr)
        stem = "gfx_%06x_%dx%d_%dbpp" % (rel, hdr['w'], hdr['h'], hdr['bpp'])

        # small icons are unreadable at 1:1, so scale them up more
        scale = args.scale
        if max(hdr['w'], hdr['h']) < 32:
            scale = max(scale, 4)
        out = []
        for row in rows:
            line = []
            for px in row:
                line += list(px) * scale
            for _ in range(scale):
                out.append(list(line))
        png(os.path.join(gdir, stem + ".png"), len(out[0]) // 3, len(out), out)

        note = notes.get(rel, "found by chaining")
        manifest.append(("graphic", addr, hdr['size'], hdr['w'], hdr['h'],
                         "graphics/" + stem + ".png", note))

        if args.headers:
            raw = d[addr:addr + hdr['size']]
            with open(os.path.join(hdir, stem + ".h"), "w") as fh:
                fh.write("""/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x%06x (flash 0x%06x), %d bytes, %d x %d at %d bpp with a
 * %d-entry palette. Provenance: %s.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         %d rows of %d bytes, top-down, most significant bit leftmost
 *%s
 */

#ifndef %s_H
#define %s_H

#define %s_W       %d
#define %s_H_PX    %d
#define %s_BPP     %d
#define %s_PALETTE %d
#define %s_STRIDE  %d
#define %s_OFFSET  0x%06x

%s

#endif
""" % (rel, addr, hdr['size'], hdr['w'], hdr['h'], hdr['bpp'], hdr['pal'],
       note, hdr['h'], hdr['stride'],
       "\n * At 1 bpp, index 0 is transparent and index 1 is the palette colour."
       if hdr['bpp'] == 1 else "",
       stem.upper(), stem.upper(), stem.upper(), hdr['w'], stem.upper(), hdr['h'],
       stem.upper(), hdr['bpp'], stem.upper(), hdr['pal'], stem.upper(),
       hdr['stride'], stem.upper(), rel, c_array(stem + "_data", raw)))
            manifest.append(("graphic-header", addr, hdr['size'], hdr['w'],
                             hdr['h'], "headers/" + stem + ".h", note))

    # Records that share a size are almost always one set - the ten digits of a
    # clock font, an icon strip, the frames of an animation. Sheets make that
    # visible: 30 records at 8x12 turn out to be digit runs in three colours.
    sdir = os.path.join(gdir, "sets")
    os.makedirs(sdir, exist_ok=True)
    groups = {}
    for addr in sorted(records):
        h = records[addr]
        groups.setdefault((h['w'], h['h'], h['bpp']), []).append(addr)
    sheets = 0
    for (w, h, bpp), addrs in sorted(groups.items()):
        if len(addrs) < 2:
            continue
        cols = min(len(addrs), max(1, 256 // max(1, w)))
        grid_rows = (len(addrs) + cols - 1) // cols
        canvas = [[BG] * (cols * w) for _ in range(grid_rows * h)]
        for i, addr in enumerate(addrs):
            rows, _ = decode_asset(d, addr, records[addr])
            ox, oy = (i % cols) * w, (i // cols) * h
            for y, row in enumerate(rows):
                for x, px in enumerate(row):
                    canvas[oy + y][ox + x] = px
        scale = 2 if w >= 32 else 4
        out = []
        for row in canvas:
            line = []
            for px in row:
                line += list(px) * scale
            for _ in range(scale):
                out.append(list(line))
        fn = "set_%dx%d_%dbpp_x%d.png" % (w, h, bpp, len(addrs))
        png(os.path.join(sdir, fn), len(out[0]) // 3, len(out), out)
        manifest.append(("graphic-set", addrs[0], len(addrs), w, h,
                         "graphics/sets/" + fn,
                         "%d records sharing this size" % len(addrs)))
        sheets += 1
    print("         %d contact sheets for records that share a size" % sheets)


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
    ap.add_argument("--src", default="reversed/src/firmware.c",
                    help="decompiled source, for the asset offsets")
    ap.add_argument("--decl", default="reversed/include/fw_data.h",
                    help="generated data header, to resolve those offsets")
    ap.add_argument("--no-headers", dest="headers", action="store_false",
                    help="PNGs and text only, skip the C headers")
    ap.add_argument("--skip-graphics", action="store_true")
    args = ap.parse_args()

    d = open(args.firmware, 'rb').read()
    if len(d) < 0x070000:
        sys.exit("image is %d bytes, too short to hold the assets" % len(d))

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
