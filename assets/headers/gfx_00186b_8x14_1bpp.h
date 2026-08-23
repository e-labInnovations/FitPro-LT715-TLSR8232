/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x00186b (flash 0x04c093), 24 bytes, 8 x 14 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001595.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         14 rows of 1 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_00186B_8X14_1BPP_H
#define GFX_00186B_8X14_1BPP_H

#define GFX_00186B_8X14_1BPP_W       8
#define GFX_00186B_8X14_1BPP_H_PX    14
#define GFX_00186B_8X14_1BPP_BPP     1
#define GFX_00186B_8X14_1BPP_PALETTE 1
#define GFX_00186B_8X14_1BPP_STRIDE  1
#define GFX_00186B_8X14_1BPP_OFFSET  0x00186b

static const unsigned char gfx_00186b_8x14_1bpp_data[24] = {
    0x08, 0x0e, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30,
};

#endif
