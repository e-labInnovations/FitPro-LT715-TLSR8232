/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x000f73 (flash 0x04b79b), 46 bytes, 32 x 9 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x000d4c.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         9 rows of 4 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_000F73_32X9_1BPP_H
#define GFX_000F73_32X9_1BPP_H

#define GFX_000F73_32X9_1BPP_W       32
#define GFX_000F73_32X9_1BPP_H_PX    9
#define GFX_000F73_32X9_1BPP_BPP     1
#define GFX_000F73_32X9_1BPP_PALETTE 1
#define GFX_000F73_32X9_1BPP_STRIDE  4
#define GFX_000F73_32X9_1BPP_OFFSET  0x000f73

static const unsigned char gfx_000f73_32x9_1bpp_data[46] = {
    0x20, 0x09, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
    0x80, 0x00, 0xec, 0xec, 0xe3, 0x80, 0x92, 0x92, 0x94, 0x80, 0x92, 0x92, 0x94, 0x80, 0x92, 0x92,
    0x94, 0x80, 0x92, 0x92, 0x93, 0x80, 0x00, 0x00, 0x04, 0x80, 0x00, 0x00, 0x03, 0x00,
};

#endif
