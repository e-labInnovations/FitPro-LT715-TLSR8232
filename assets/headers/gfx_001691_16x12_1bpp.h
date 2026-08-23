/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x001691 (flash 0x04beb9), 34 bytes, 16 x 12 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001595.
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

#ifndef GFX_001691_16X12_1BPP_H
#define GFX_001691_16X12_1BPP_H

#define GFX_001691_16X12_1BPP_W       16
#define GFX_001691_16X12_1BPP_H_PX    12
#define GFX_001691_16X12_1BPP_BPP     1
#define GFX_001691_16X12_1BPP_PALETTE 1
#define GFX_001691_16X12_1BPP_STRIDE  2
#define GFX_001691_16X12_1BPP_OFFSET  0x001691

static const unsigned char gfx_001691_16x12_1bpp_data[34] = {
    0x10, 0x0c, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xf1, 0x00, 0x93, 0x00, 0x92, 0x00,
    0xf6, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x08, 0x00, 0x18, 0x00, 0x13, 0xc0, 0x32, 0x40, 0x22, 0x40,
    0x63, 0xc0,
};

#endif
