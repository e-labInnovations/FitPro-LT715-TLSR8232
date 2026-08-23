/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x001883 (flash 0x04c0ab), 38 bytes, 16 x 14 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001595.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         14 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_001883_16X14_1BPP_H
#define GFX_001883_16X14_1BPP_H

#define GFX_001883_16X14_1BPP_W       16
#define GFX_001883_16X14_1BPP_H_PX    14
#define GFX_001883_16X14_1BPP_BPP     1
#define GFX_001883_16X14_1BPP_PALETTE 1
#define GFX_001883_16X14_1BPP_STRIDE  2
#define GFX_001883_16X14_1BPP_OFFSET  0x001883

static const unsigned char gfx_001883_16x14_1bpp_data[38] = {
    0x10, 0x0e, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc0, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

#endif
