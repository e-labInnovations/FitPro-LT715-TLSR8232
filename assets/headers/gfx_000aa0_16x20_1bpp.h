/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x000aa0 (flash 0x04b2c8), 50 bytes, 16 x 20 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x0009d3.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         20 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_000AA0_16X20_1BPP_H
#define GFX_000AA0_16X20_1BPP_H

#define GFX_000AA0_16X20_1BPP_W       16
#define GFX_000AA0_16X20_1BPP_H_PX    20
#define GFX_000AA0_16X20_1BPP_BPP     1
#define GFX_000AA0_16X20_1BPP_PALETTE 1
#define GFX_000AA0_16X20_1BPP_STRIDE  2
#define GFX_000AA0_16X20_1BPP_OFFSET  0x000aa0

static const unsigned char gfx_000aa0_16x20_1bpp_data[50] = {
    0x10, 0x14, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1b, 0xbe, 0x0f, 0xc0, 0x1f, 0xc0, 0x3f, 0xc0,
    0x3e, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0xff, 0xc0, 0xff, 0xc0, 0xff, 0xc0, 0x3c, 0x00,
    0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00,
    0x3c, 0x00,
};

#endif
