/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x000915 (flash 0x04b13d), 34 bytes, 16 x 12 at 1 bpp with a
 * 1-entry palette. Provenance: direct: DAT_00008e58.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         12 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_000915_16X12_1BPP_H
#define GFX_000915_16X12_1BPP_H

#define GFX_000915_16X12_1BPP_W       16
#define GFX_000915_16X12_1BPP_H_PX    12
#define GFX_000915_16X12_1BPP_BPP     1
#define GFX_000915_16X12_1BPP_PALETTE 1
#define GFX_000915_16X12_1BPP_STRIDE  2
#define GFX_000915_16X12_1BPP_OFFSET  0x000915

static const unsigned char gfx_000915_16x12_1bpp_data[34] = {
    0x10, 0x0c, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x81, 0x00, 0xc1, 0x80, 0xf1, 0xe0,
    0xf9, 0xf0, 0xff, 0xfc, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfc, 0xf9, 0xf0, 0xf1, 0xe0, 0xc1, 0x80,
    0x81, 0x00,
};

#endif
