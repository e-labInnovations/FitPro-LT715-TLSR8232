/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x001254 (flash 0x04ba7c), 28 bytes, 16 x 9 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001190.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         9 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_001254_16X9_1BPP_H
#define GFX_001254_16X9_1BPP_H

#define GFX_001254_16X9_1BPP_W       16
#define GFX_001254_16X9_1BPP_H_PX    9
#define GFX_001254_16X9_1BPP_BPP     1
#define GFX_001254_16X9_1BPP_PALETTE 1
#define GFX_001254_16X9_1BPP_STRIDE  2
#define GFX_001254_16X9_1BPP_OFFSET  0x001254

static const unsigned char gfx_001254_16x9_1bpp_data[28] = {
    0x10, 0x09, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x78, 0x78, 0x84, 0x84,
    0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x8c, 0x8c, 0x7c, 0x7c, 0x02, 0x02,
};

#endif
