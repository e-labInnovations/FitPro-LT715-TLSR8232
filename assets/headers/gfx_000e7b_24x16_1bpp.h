/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x000e7b (flash 0x04b6a3), 58 bytes, 24 x 16 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x000d4c.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         16 rows of 3 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_000E7B_24X16_1BPP_H
#define GFX_000E7B_24X16_1BPP_H

#define GFX_000E7B_24X16_1BPP_W       24
#define GFX_000E7B_24X16_1BPP_H_PX    16
#define GFX_000E7B_24X16_1BPP_BPP     1
#define GFX_000E7B_24X16_1BPP_PALETTE 1
#define GFX_000E7B_24X16_1BPP_STRIDE  3
#define GFX_000E7B_24X16_1BPP_OFFSET  0x000e7b

static const unsigned char gfx_000e7b_24x16_1bpp_data[58] = {
    0x18, 0x10, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x56, 0x60, 0x0f, 0xc0, 0x00, 0x3f, 0xf0, 0x00,
    0x7f, 0xf8, 0x00, 0xf3, 0x3c, 0x00, 0xf3, 0x30, 0x00, 0xff, 0xcf, 0x00, 0xff, 0xbf, 0xc0, 0xff,
    0x7f, 0xe0, 0x7e, 0xe6, 0x70, 0x7e, 0xe6, 0x70, 0x3e, 0xff, 0xf0, 0x3e, 0xff, 0xf0, 0x20, 0xff,
    0xe0, 0x00, 0x7f, 0xe0, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x40,
};

#endif
