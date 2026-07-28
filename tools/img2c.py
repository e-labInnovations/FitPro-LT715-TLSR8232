#!/usr/bin/env python3
"""
img2c.py — convert a PNG into a C header for the ST7735 on the FitPro LT715.

The display's color order is swapped (see display_color() in lib/display/display.c),
so pixels are emitted as BGR565: bbbbbggg gggrrrrr.

Output kinds:
  raw   uint16_t NAME_data[W*H]              — draw with display_draw_image()
  rle   uint16_t NAME_rle[2*runs]            — draw with display_draw_image_rle()
  mask  uint8_t  NAME_mask[]  (1bpp, LSB first, from alpha)
                                             — draw with gfx_draw_rgb_bitmap_with_mask()

Examples:
  ./img2c.py logo.png            --name logo  --rle          -o logo.h
  ./img2c.py heart.png --size 32 --name heart --alpha-mask   -o heart.h

Pure stdlib — no Pillow. Supports 8-bit PNGs of color type 0/2/3/4/6.
"""

import argparse
import struct
import sys
import zlib


# ---------------------------------------------------------------------------
# Minimal PNG decoder -> (width, height, rgba bytes)
# ---------------------------------------------------------------------------

def png_decode(path):
    with open(path, "rb") as f:
        data = f.read()

    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG file")

    pos = 8
    width = height = bitdepth = colortype = interlace = None
    palette = b""
    trns = b""
    idat = bytearray()

    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        ctype = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length  # length + type + body + crc

        if ctype == b"IHDR":
            width, height, bitdepth, colortype, _comp, _filt, interlace = \
                struct.unpack(">IIBBBBB", body)
        elif ctype == b"PLTE":
            palette = body
        elif ctype == b"tRNS":
            trns = body
        elif ctype == b"IDAT":
            idat += body
        elif ctype == b"IEND":
            break

    if bitdepth != 8:
        raise ValueError(f"{path}: only 8-bit PNGs supported (got {bitdepth}-bit). "
                         "Re-export as 8-bit RGB/RGBA.")
    if interlace:
        raise ValueError(f"{path}: interlaced PNGs not supported. Re-export without Adam7.")

    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(colortype)
    if channels is None:
        raise ValueError(f"{path}: unsupported color type {colortype}")

    raw = zlib.decompress(bytes(idat))
    stride = width * channels
    out = bytearray(stride * height)
    prev = bytearray(stride)

    # Undo the per-scanline filters (PNG spec section 9)
    src = 0
    for y in range(height):
        ftype = raw[src]
        src += 1
        line = bytearray(raw[src:src + stride])
        src += stride

        if ftype == 1:      # Sub
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 0xFF
        elif ftype == 2:    # Up
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif ftype == 3:    # Average
            for i in range(stride):
                a = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((a + prev[i]) >> 1)) & 0xFF
        elif ftype == 4:    # Paeth
            for i in range(stride):
                a = line[i - channels] if i >= channels else 0
                b = prev[i]
                c = prev[i - channels] if i >= channels else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 0xFF
        elif ftype != 0:
            raise ValueError(f"{path}: bad filter type {ftype} on row {y}")

        out[y * stride:(y + 1) * stride] = line
        prev = line

    # Expand whatever we got into straight RGBA
    rgba = bytearray(width * height * 4)
    for i in range(width * height):
        s = i * channels
        d = i * 4
        if colortype == 0:      # grey
            g = out[s]
            rgba[d:d + 4] = bytes((g, g, g, 255))
        elif colortype == 4:    # grey + alpha
            g = out[s]
            rgba[d:d + 4] = bytes((g, g, g, out[s + 1]))
        elif colortype == 2:    # rgb
            rgba[d:d + 3] = out[s:s + 3]
            rgba[d + 3] = 255
        elif colortype == 6:    # rgba
            rgba[d:d + 4] = out[s:s + 4]
        else:                   # palette
            idx = out[s]
            rgba[d:d + 3] = palette[idx * 3:idx * 3 + 3]
            rgba[d + 3] = trns[idx] if idx < len(trns) else 255

    return width, height, rgba


# ---------------------------------------------------------------------------
# Resize (nearest neighbor) and color conversion
# ---------------------------------------------------------------------------

def resize_nearest(src, sw, sh, dw, dh):
    dst = bytearray(dw * dh * 4)
    for y in range(dh):
        sy = y * sh // dh
        for x in range(dw):
            sx = x * sw // dw
            s = (sy * sw + sx) * 4
            d = (y * dw + x) * 4
            dst[d:d + 4] = src[s:s + 4]
    return dst


def resize_box(src, sw, sh, dw, dh):
    """Area-average downscale. Averages in premultiplied alpha so that
    transparent pixels don't bleed their (meaningless) colour into edges."""
    dst = bytearray(dw * dh * 4)
    for y in range(dh):
        y0, y1 = y * sh // dh, max(y * sh // dh + 1, (y + 1) * sh // dh)
        for x in range(dw):
            x0, x1 = x * sw // dw, max(x * sw // dw + 1, (x + 1) * sw // dw)
            ar = ag = ab = aa = 0
            n = 0
            for sy in range(y0, y1):
                row = sy * sw
                for sx in range(x0, x1):
                    s = (row + sx) * 4
                    a = src[s + 3]
                    ar += src[s] * a
                    ag += src[s + 1] * a
                    ab += src[s + 2] * a
                    aa += a
                    n += 1
            d = (y * dw + x) * 4
            if aa:
                dst[d] = min(255, ar // aa)
                dst[d + 1] = min(255, ag // aa)
                dst[d + 2] = min(255, ab // aa)
                dst[d + 3] = aa // n
            # else leave fully transparent black
    return dst


def bgr565(r, g, b):
    """Match display_color() in lib/display/display.c — R and B are swapped."""
    return ((b & 0xF8) << 8) | ((r & 0xFC) << 3) | ((g & 0xF8) >> 3)


def to_pixels(rgba, count, bg):
    """RGBA -> list of BGR565, compositing transparent pixels onto bg."""
    px = []
    for i in range(count):
        r, g, b, a = rgba[i * 4:i * 4 + 4]
        if a < 255:
            r = (r * a + bg[0] * (255 - a)) // 255
            g = (g * a + bg[1] * (255 - a)) // 255
            b = (b * a + bg[2] * (255 - a)) // 255
        px.append(bgr565(r, g, b))
    return px


def to_mask(rgba, w, h, threshold):
    """Alpha channel -> 1bpp rows, LSB first (the order gfx.c walks bits in)."""
    stride = (w + 7) // 8
    mask = bytearray(stride * h)
    for y in range(h):
        for x in range(w):
            if rgba[(y * w + x) * 4 + 3] >= threshold:
                mask[y * stride + (x >> 3)] |= 1 << (x & 7)
    return mask


def rle_encode(px):
    """(count, color) pairs. Runs are capped at 65535 pixels."""
    runs = []
    i = 0
    while i < len(px):
        c = px[i]
        n = 1
        while i + n < len(px) and px[i + n] == c and n < 0xFFFF:
            n += 1
        runs.append((n, c))
        i += n
    return runs


# ---------------------------------------------------------------------------
# Emit
# ---------------------------------------------------------------------------

def fmt_u16_array(values, per_line=12, indent="    "):
    lines = []
    for i in range(0, len(values), per_line):
        chunk = ", ".join(f"0x{v:04X}" for v in values[i:i + per_line])
        lines.append(indent + chunk + ",")
    return "\n".join(lines)


def fmt_u8_array(values, per_line=16, indent="    "):
    lines = []
    for i in range(0, len(values), per_line):
        chunk = ", ".join(f"0x{v:02X}" for v in values[i:i + per_line])
        lines.append(indent + chunk + ",")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", help="source .png")
    ap.add_argument("-o", "--output", help="output .h (default: stdout)")
    ap.add_argument("-n", "--name", help="C identifier prefix (default: file stem)")
    ap.add_argument("--size", type=int, help="resize to SIZE x SIZE")
    ap.add_argument("--width", type=int, help="resize width (height follows the aspect ratio)")
    ap.add_argument("--height", type=int, help="resize height (width follows the aspect ratio)")
    ap.add_argument("--filter", choices=("auto", "box", "nearest"), default="auto",
                    help="resampling: box (area average) when shrinking, "
                         "nearest when enlarging (default auto)")
    ap.add_argument("--rle", action="store_true",
                    help="emit run-length encoded data instead of a raw array")
    ap.add_argument("--alpha-mask", action="store_true",
                    help="also emit a 1bpp transparency mask from the alpha channel")
    ap.add_argument("--alpha-threshold", type=int, default=128,
                    help="alpha >= this counts as opaque (default 128)")
    ap.add_argument("--bg", default="000000",
                    help="hex RGB that transparent pixels blend into (default 000000)")
    args = ap.parse_args()

    name = args.name or args.input.split("/")[-1].rsplit(".", 1)[0]
    name = "".join(ch if ch.isalnum() else "_" for ch in name)
    bg = tuple(int(args.bg[i:i + 2], 16) for i in (0, 2, 4))

    w, h, rgba = png_decode(args.input)

    if args.size:
        dw = dh = args.size
    elif args.width and args.height:
        dw, dh = args.width, args.height
    elif args.width:                       # keep the aspect ratio
        dw, dh = args.width, max(1, round(h * args.width / w))
    elif args.height:
        dw, dh = max(1, round(w * args.height / h)), args.height
    else:
        dw, dh = w, h

    if (dw, dh) != (w, h):
        shrinking = dw <= w and dh <= h
        use_box = args.filter == "box" or (args.filter == "auto" and shrinking)
        rgba = (resize_box if use_box else resize_nearest)(rgba, w, h, dw, dh)
        w, h = dw, dh

    px = to_pixels(rgba, w * h, bg)
    raw_bytes = w * h * 2

    out = []
    guard = name.upper() + "_H"
    out.append(f"// Generated by tools/img2c.py from {args.input} — do not edit by hand.")
    out.append(f"// {w}x{h}, BGR565 (channel order matches display_color()).")
    out.append(f"#ifndef {guard}")
    out.append(f"#define {guard}")
    out.append("")
    out.append("#include <stdint.h>")
    out.append("")
    out.append(f"#define {name.upper()}_WIDTH  {w}")
    out.append(f"#define {name.upper()}_HEIGHT {h}")
    out.append("")

    if args.rle:
        runs = rle_encode(px)
        flat = [v for run in runs for v in run]
        out.append(f"#define {name.upper()}_RLE_RUNS {len(runs)}")
        out.append("")
        out.append(f"// {len(runs)} runs = {len(flat) * 2} bytes "
                   f"({raw_bytes} bytes raw, {100 - len(flat) * 200 // raw_bytes}% saved)")
        out.append(f"static const uint16_t {name}_rle[{len(flat)}] = {{")
        out.append(fmt_u16_array(flat))
        out.append("};")
        size_note = len(flat) * 2
    else:
        out.append(f"// {raw_bytes} bytes of flash")
        out.append(f"static const uint16_t {name}_data[{w * h}] = {{")
        out.append(fmt_u16_array(px))
        out.append("};")
        size_note = raw_bytes

    if args.alpha_mask:
        mask = to_mask(rgba, w, h, args.alpha_threshold)
        out.append("")
        out.append(f"// 1bpp transparency mask, {len(mask)} bytes")
        out.append(f"static const uint8_t {name}_mask[{len(mask)}] = {{")
        out.append(fmt_u8_array(mask))
        out.append("};")
        size_note += len(mask)

    out.append("")
    out.append(f"#endif // {guard}")
    out.append("")
    text = "\n".join(out)

    if args.output:
        with open(args.output, "w") as f:
            f.write(text)
        print(f"{args.input}: {w}x{h} -> {args.output} ({size_note} bytes)", file=sys.stderr)
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
