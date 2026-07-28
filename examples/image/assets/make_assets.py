#!/usr/bin/env python3
"""
Generates the sample artwork for examples/image so the example has something to
show without shipping binary art from elsewhere. Pure stdlib, no Pillow.

  ./make_assets.py          # writes sunset.png (128x128) and heart.png (32x32 RGBA)
"""

import struct
import zlib


def write_png(path, width, height, pixels, alpha=False):
    """pixels: list of (r,g,b) or (r,g,b,a) tuples, row-major."""
    ch = 4 if alpha else 3
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter type 0 (None)
        for x in range(width):
            raw.extend(pixels[y * width + x][:ch])

    def chunk(tag, body):
        return (struct.pack(">I", len(body)) + tag + body +
                struct.pack(">I", zlib.crc32(tag + body) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6 if alpha else 2, 0, 0, 0)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        f.write(chunk(b"IEND", b""))
    print(f"wrote {path} ({width}x{height})")


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def sunset(size=128):
    """Banded sky + sun + mountains — lots of flat color, so it RLEs well."""
    top, mid, low = (26, 26, 74), (204, 92, 84), (255, 176, 92)
    horizon = int(size * 0.68)
    sun_x, sun_y, sun_r = size // 2, int(size * 0.52), 20
    px = []

    for y in range(size):
        # Sky: two-stop gradient, quantized into 5px bands
        band = (y // 5) * 5
        t = band / horizon if horizon else 0
        sky = lerp(top, mid, min(t, 1.0) * 2) if t < 0.5 else lerp(mid, low, (t - 0.5) * 2)

        for x in range(size):
            if y >= horizon:
                c = (18, 20, 40)
                # Two mountain ridges rising out of the horizon
                left = horizon - max(0, 26 - abs(x - 34) * 2)
                right = horizon - max(0, 34 - abs(x - 92) * 3 // 2)
                if y >= horizon - 2:
                    c = (14, 14, 30)
                if y < min(left, right):
                    c = sky
                elif y < horizon:
                    c = (40, 34, 70)
            else:
                dx, dy = x - sun_x, y - sun_y
                if dx * dx + dy * dy <= sun_r * sun_r:
                    # Sun, with a couple of darker scan bands across it
                    c = (255, 232, 150) if (y % 8) < 6 else (250, 190, 110)
                else:
                    c = sky
            px.append(c)

    # Mountain silhouettes drawn on top of the sky, above the horizon line
    for x in range(size):
        peak_l = max(0, 30 - abs(x - 34) * 2)
        peak_r = max(0, 40 - abs(x - 92) * 3 // 2)
        peak = max(peak_l, peak_r)
        for y in range(horizon - peak, horizon):
            if 0 <= y < size:
                px[y * size + x] = (34, 30, 62) if peak == peak_l else (24, 22, 48)

    return px


def heart(size=32):
    """Filled heart with a transparent background and a soft highlight."""
    px = []
    for y in range(size):
        for x in range(size):
            # Normalized coords, heart curve: (x^2+y^2-1)^3 - x^2*y^3 <= 0
            nx = (x - size / 2 + 0.5) / (size * 0.46)
            ny = -(y - size / 2 + 0.5) / (size * 0.42) + 0.25
            v = (nx * nx + ny * ny - 1) ** 3 - nx * nx * ny ** 3
            if v <= 0:
                shade = 0.65 + 0.35 * max(0.0, 1.0 - ((nx + 0.3) ** 2 + (ny - 0.45) ** 2))
                c = (int(min(255, 235 * shade)), int(60 * shade), int(80 * shade), 255)
            else:
                c = (0, 0, 0, 0)
            px.append(c)
    return px


if __name__ == "__main__":
    write_png("sunset.png", 128, 128, sunset(128))
    write_png("heart.png", 32, 32, heart(32), alpha=True)
