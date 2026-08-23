/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x001190 (flash 0x04b9b8), 26 bytes, 128 x 1 at 1 bpp with a
 * 1-entry palette. Provenance: direct: PTR_FUN_000086c0.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         1 rows of 16 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_001190_128X1_1BPP_H
#define GFX_001190_128X1_1BPP_H

#define GFX_001190_128X1_1BPP_W       128
#define GFX_001190_128X1_1BPP_H_PX    1
#define GFX_001190_128X1_1BPP_BPP     1
#define GFX_001190_128X1_1BPP_PALETTE 1
#define GFX_001190_128X1_1BPP_STRIDE  16
#define GFX_001190_128X1_1BPP_OFFSET  0x001190

static const unsigned char gfx_001190_128x1_1bpp_data[26] = {
    0x80, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

#endif
