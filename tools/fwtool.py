#!/usr/bin/env python3
"""
Reverse-engineering helper for FitPro LT716 stock firmware images.

Everything here was derived from binaries/stock/LT716_V10712_211091429.bin and
verified by rendering the result back out: the fonts read as legible text, the
CJK codepoint table matches Unicode, and the string table matches the UI.

Layout of that image (see `map`):

    0x000000-0x01a8a4  main application   (Telink KNLT image, length at 0x18)
    0x01a8a4-0x020000  erased padding
    0x020000-0x028c74  second application (KNLT image, 36 KB)
    0x028c74-0x029000  erased padding
    0x029011-0x031c58  font, U+0001..U+07CB   1995 glyphs
    0x031c58-0x0339b0  CJK codepoint index    3756 x uint16, sorted
    0x0339b0-0x0441c8  font, CJK              3756 glyphs, same format
    0x0441c8-0x044200  string-table header    magic 0xBEEF
    0x044200-0x04a860  UI strings             7 languages x 78 slots
    0x04a860-0x06fc04  not yet identified     ~151 KB
    0x06fc04-0x07d000  erased

Glyph format, both fonts: a 12x12 cell in 18 bytes.
    bytes 0..11   left 8 columns, one byte per row, MSB = leftmost pixel
    bytes 12..17  right 4 columns, two rows per byte,
                  high nibble = even row, low nibble = odd row
Latin glyphs leave the right-hand plane zero, which is why they also render
correctly if you (wrongly) read the glyph as 8 wide by 18 tall.
"""

import argparse, struct, sys, zlib

FONT_BASE   = 0x028fff   # addr(codepoint) = FONT_BASE + cp*18
CJK_TABLE   = 0x031c58
CJK_COUNT   = 3756
CJK_GLYPHS  = 0x0339b0
STR_MAGIC   = 0x0441c8
STR_BASE    = 0x044200
STR_SLOT    = 0x30
STR_STRIDE  = 0xea0
STR_SLOTS   = STR_STRIDE // STR_SLOT
GLYPH_SIZE  = 18


def load(path):
    return open(path, 'rb').read()


def glyph_rows(d, off):
    """Returns 12 strings of 12 chars, '#' for ink."""
    rows = []
    for r in range(12):
        left = format(d[off + r], '08b')
        b = d[off + 12 + r // 2]
        nib = (b >> 4) if r % 2 == 0 else (b & 0x0f)
        rows.append((left + format(nib, '04b')).replace('0', '.').replace('1', '#'))
    return rows


def cjk_index(d, cp):
    """Binary search the codepoint table, as the firmware itself must do."""
    lo, hi = 0, CJK_COUNT - 1
    while lo <= hi:
        mid = (lo + hi) // 2
        v = struct.unpack_from('<H', d, CJK_TABLE + 2 * mid)[0]
        if v == cp: return mid
        if v < cp: lo = mid + 1
        else: hi = mid - 1
    return None


def glyph_addr(d, cp):
    if cp <= 0x07cb:
        return FONT_BASE + cp * GLYPH_SIZE
    i = cjk_index(d, cp)
    return None if i is None else CJK_GLYPHS + i * GLYPH_SIZE


def slot_text(d, off):
    raw = d[off:off + STR_SLOT]
    out = []
    for i in range(0, len(raw) - 1, 2):
        v = raw[i] | (raw[i + 1] << 8)
        if v == 0: break
        out.append(chr(v))
    return ''.join(out)


def png(path, w, h, rows):
    def chunk(t, data):
        return (struct.pack('>I', len(data)) + t + data
                + struct.pack('>I', zlib.crc32(t + data) & 0xffffffff))
    raw = b''.join(b'\x00' + bytes(r) for r in rows)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n'
        + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
        + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def cmd_map(d, args):
    print("image: %d bytes (0x%x)" % (len(d), len(d)))
    i = 0
    while True:
        i = d.find(b'KNLT', i)
        if i < 0: break
        base = i - 8
        n = struct.unpack_from('<I', d, base + 0x18)[0]
        print("  0x%06x  KNLT application, length 0x%x -> ends 0x%06x"
              % (base, n, base + n))
        i += 4
    print("  0x%06x  font, U+0001..U+07CB (%d glyphs)"
          % (FONT_BASE + GLYPH_SIZE, (CJK_TABLE - FONT_BASE - GLYPH_SIZE)//GLYPH_SIZE))
    print("  0x%06x  CJK codepoint index, %d x uint16" % (CJK_TABLE, CJK_COUNT))
    print("  0x%06x  CJK glyphs, %d x %d B -> ends 0x%06x"
          % (CJK_GLYPHS, CJK_COUNT, GLYPH_SIZE, CJK_GLYPHS + CJK_COUNT*GLYPH_SIZE))
    print("  0x%06x  string table, magic 0x%04X"
          % (STR_MAGIC, struct.unpack_from('<H', d, STR_MAGIC)[0]))
    langs = 0
    while slot_text(d, STR_BASE + langs*STR_STRIDE).strip():
        langs += 1
    print("  0x%06x  %d language blocks x %d slots -> ends 0x%06x"
          % (STR_BASE, langs, STR_SLOTS, STR_BASE + langs*STR_STRIDE))
    a = STR_BASE + langs*STR_STRIDE
    print("  0x%06x  1bpp graphics, %d bytes (battery icons first, 56 px wide)"
          % (a, 0x06fc04 - a))
    print("  0x%06x  erased to end" % 0x06fc04)


def cmd_glyph(d, args):
    for spec in args.codepoints:
        t = spec.lower()
        if t.startswith('u+'):   cp = int(spec[2:], 16)
        elif t.startswith('0x'): cp = int(spec[2:], 16)
        elif len(spec) == 1:     cp = ord(spec)
        else:                    cp = int(spec, 16)
        a = glyph_addr(d, cp)
        if a is None:
            print("U+%04X %r: not in this firmware" % (cp, chr(cp))); continue
        print("U+%04X %r @0x%06x" % (cp, chr(cp), a))
        for r in glyph_rows(d, a): print("   " + r)


def cmd_strings(d, args):
    langs = 0
    while slot_text(d, STR_BASE + langs*STR_STRIDE).strip():
        langs += 1
    for s in range(STR_SLOTS):
        vals = [slot_text(d, STR_BASE + l*STR_STRIDE + s*STR_SLOT) for l in range(langs)]
        if not any(v.strip() for v in vals): continue
        print("slot %2d: %s" % (s, " | ".join(v or "-" for v in vals)))


def cmd_cjk(d, args):
    cps = [struct.unpack_from('<H', d, CJK_TABLE + 2*i)[0] for i in range(CJK_COUNT)]
    print("%d codepoints, U+%04X..U+%04X" % (len(cps), cps[0], cps[-1]))
    line = ""
    for i, c in enumerate(cps[:args.count]):
        line += chr(c)
        if len(line) == 64:
            print("  " + line); line = ""
    if line: print("  " + line)


def cmd_width(d, args):
    """Row width of 1bpp graphics is not stored anywhere obvious, but rows of the
    same shape repeat, so autocorrelation finds it."""
    win = d[args.off:args.off + args.window]
    out = []
    for lag in range(2, args.max_width + 1):
        same = sum(1 for i in range(len(win) - lag) if win[i] == win[i + lag])
        out.append((same / (len(win) - lag), lag))
    out.sort(reverse=True)
    print("best row strides at 0x%06x (harmonics of the true width also score high):"
          % args.off)
    for score, lag in out[:6]:
        print("  %2d bytes = %3d px   %.3f" % (lag, lag*8, score))


def cmd_bitmap(d, args):
    """Render 1bpp graphics. Width is in bytes; 7 (56 px) is right for the
    battery icons at 0x04a860."""
    sc = args.scale
    rows = []
    for y in range(args.rows):
        line = []
        for xb in range(args.width):
            i = args.off + y*args.width + xb
            byte = d[i] if i < len(d) else 0
            for bit in range(8):
                line += ([255,255,255] if (byte >> (7-bit)) & 1 else [20,20,20]) * sc
        for _ in range(sc): rows.append(list(line))
    png(args.out, len(rows[0])//3, len(rows), rows)
    print("wrote %s (%d px wide, %d rows from 0x%06x)"
          % (args.out, args.width*8, args.rows, args.off))


def cmd_atlas(d, args):
    start, count, cols, sc = args.start, args.count, args.cols, args.scale
    rows = []
    r = 0
    while r*cols < count:
        band = [[] for _ in range(12*sc)]
        for c in range(cols):
            gi = r*cols + c
            if args.cjk:
                a = CJK_GLYPHS + (start + gi)*GLYPH_SIZE
            else:
                a = FONT_BASE + (start + gi)*GLYPH_SIZE
            g = glyph_rows(d, a) if a + GLYPH_SIZE <= len(d) else ["."*12]*12
            for y in range(12):
                line = []
                for ch in g[y]:
                    line += ([255,255,255] if ch == '#' else [22,22,22]) * sc
                line += [0,0,150]*sc
                for s in range(sc): band[y*sc+s] += line
        rows.extend(band)
        for _ in range(sc): rows.append([0,0,150]*(len(band[0])//3))
        r += 1
    png(args.out, len(rows[0])//3, len(rows), rows)
    print("wrote %s (%d glyphs from index %d)" % (args.out, count, start))


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('image')
    sub = p.add_subparsers(dest='cmd', required=True)

    sub.add_parser('map', help="print the flash layout").set_defaults(func=cmd_map)

    g = sub.add_parser('glyph', help="print glyphs as ASCII art")
    g.add_argument('codepoints', nargs='+', help="U+4E00, 0x41, or a literal character")
    g.set_defaults(func=cmd_glyph)

    s = sub.add_parser('strings', help="dump the UI string table, all languages")
    s.set_defaults(func=cmd_strings)

    c = sub.add_parser('cjk', help="dump the CJK codepoint index")
    c.add_argument('--count', type=int, default=256)
    c.set_defaults(func=cmd_cjk)

    w = sub.add_parser('width', help="guess the row width of 1bpp graphics")
    w.add_argument('off', type=lambda v: int(v, 0))
    w.add_argument('--window', type=lambda v: int(v, 0), default=0x800)
    w.add_argument('--max-width', type=int, default=64)
    w.set_defaults(func=cmd_width)

    b = sub.add_parser('bitmap', help="render 1bpp graphics to PNG")
    b.add_argument('out')
    b.add_argument('--off', type=lambda v: int(v, 0), default=0x04a860)
    b.add_argument('--width', type=int, default=7, help="bytes per row (7 = 56 px)")
    b.add_argument('--rows', type=int, default=180)
    b.add_argument('--scale', type=int, default=3)
    b.set_defaults(func=cmd_bitmap)

    a = sub.add_parser('atlas', help="render a glyph atlas to PNG")
    a.add_argument('out')
    a.add_argument('--start', type=int, default=32)
    a.add_argument('--count', type=int, default=320)
    a.add_argument('--cols', type=int, default=32)
    a.add_argument('--scale', type=int, default=2)
    a.add_argument('--cjk', action='store_true', help="index into the CJK font instead")
    a.set_defaults(func=cmd_atlas)

    args = p.parse_args()
    args.func(load(args.image), args)


if __name__ == '__main__':
    main()
