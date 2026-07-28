#!/usr/bin/env python3
"""
Renders "www.elabins.com" into url.png as a tiny pixel-font strip.

Why not draw it with gfx_print()? The two fonts in lib/fonts are too big:
"www.elabins.com" measures 165 px in FreeMono9pt7b and 181 px in FreeSans12pt7b,
on a 128 px wide panel. This 7 px tall hand-drawn face fits in ~90 px, and
baking it into an image means one RLE blit instead of 15 glyph draws.

  ./make_url.py            # writes url.png
"""

import struct
import zlib

# 5x7 glyphs, only the characters the URL needs. Row 6 is the baseline.
GLYPHS = {
    "w": ("     ",
          "     ",
          "#   #",
          "#   #",
          "# # #",
          "# # #",
          " # # "),
    "e": ("     ",
          "     ",
          " ### ",
          "#   #",
          "#####",
          "#    ",
          " ### "),
    "l": (" ##  ",
          "  #  ",
          "  #  ",
          "  #  ",
          "  #  ",
          "  #  ",
          " ### "),
    "a": ("     ",
          "     ",
          " ### ",
          "    #",
          " ####",
          "#   #",
          " ####"),
    "b": ("#    ",
          "#    ",
          "#### ",
          "#   #",
          "#   #",
          "#   #",
          "#### "),
    "i": ("  #  ",
          "     ",
          " ##  ",
          "  #  ",
          "  #  ",
          "  #  ",
          " ### "),
    "n": ("     ",
          "     ",
          "#### ",
          "#   #",
          "#   #",
          "#   #",
          "#   #"),
    "s": ("     ",
          "     ",
          " ####",
          "#    ",
          " ### ",
          "    #",
          "#### "),
    "c": ("     ",
          "     ",
          " ### ",
          "#    ",
          "#    ",
          "#    ",
          " ### "),
    "o": ("     ",
          "     ",
          " ### ",
          "#   #",
          "#   #",
          "#   #",
          " ### "),
    "m": ("     ",
          "     ",
          "## # ",
          "# # #",
          "# # #",
          "# # #",
          "# # #"),
    ".": ("     ",
          "     ",
          "     ",
          "     ",
          "     ",
          "     ",
          "  #  "),
}

TEXT = "www.elabins.com"
FG = (245, 245, 245)   # near-white
BG = (0, 0, 0)         # matches the splash background
GAP = 1                # blank columns between glyphs


def trim(rows):
    """Drop empty leading/trailing columns so the face is proportional."""
    cols = [c for c in range(5) if any(r[c] == "#" for r in rows)]
    if not cols:
        return ["  "], 2   # space-ish (only '.' has content, so this is unused)
    lo, hi = min(cols), max(cols)
    return [r[lo:hi + 1] for r in rows], hi - lo + 1


def render(text):
    glyphs = [trim(GLYPHS[ch]) for ch in text]
    width = sum(w for _, w in glyphs) + GAP * (len(glyphs) - 1)
    height = 7
    px = [BG] * (width * height)

    x = 0
    for rows, w in glyphs:
        for y in range(height):
            for i in range(w):
                if rows[y][i] == "#":
                    px[y * width + x + i] = FG
        x += w + GAP
    return width, height, px


def write_png(path, width, height, pixels):
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            raw.extend(pixels[y * width + x])

    def chunk(tag, body):
        return (struct.pack(">I", len(body)) + tag + body +
                struct.pack(">I", zlib.crc32(tag + body) & 0xFFFFFFFF))

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        f.write(chunk(b"IEND", b""))
    print(f"wrote {path} ({width}x{height})")


if __name__ == "__main__":
    w, h, px = render(TEXT)
    write_png("url.png", w, h, px)
