/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x001058 (flash 0x04b880), 40 bytes, 16 x 15 at 1 bpp with a
 * 1-entry palette. Provenance: direct: PTR_PTR_00008d00.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         15 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_001058_16X15_1BPP_H
#define GFX_001058_16X15_1BPP_H

#define GFX_001058_16X15_1BPP_W       16
#define GFX_001058_16X15_1BPP_H_PX    15
#define GFX_001058_16X15_1BPP_BPP     1
#define GFX_001058_16X15_1BPP_PALETTE 1
#define GFX_001058_16X15_1BPP_STRIDE  2
#define GFX_001058_16X15_1BPP_OFFSET  0x001058

static const unsigned char gfx_001058_16x15_1bpp_data[40] = {
    0x10, 0x0f, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1c, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x1c, 0x00,
    0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

#endif
