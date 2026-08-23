/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x000000 (flash 0x04a828), 22 bytes, 8 x 12 at 1 bpp with a
 * 1-entry palette. Provenance: found by chaining.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         12 rows of 1 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_000000_8X12_1BPP_H
#define GFX_000000_8X12_1BPP_H

#define GFX_000000_8X12_1BPP_W       8
#define GFX_000000_8X12_1BPP_H_PX    12
#define GFX_000000_8X12_1BPP_BPP     1
#define GFX_000000_8X12_1BPP_PALETTE 1
#define GFX_000000_8X12_1BPP_STRIDE  1
#define GFX_000000_8X12_1BPP_OFFSET  0x000000

static const unsigned char gfx_000000_8x12_1bpp_data[22] = {
    0x08, 0x0c, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x3f, 0x6c, 0x6c, 0x6c, 0x6c, 0xfe, 0xfe,
    0xfe, 0x7c, 0x38, 0x10, 0x10, 0x10,
};

#endif
