/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x0009af (flash 0x04b1d7), 36 bytes, 16 x 13 at 1 bpp with a
 * 1-entry palette. Provenance: direct: DAT_00008e60.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         13 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_0009AF_16X13_1BPP_H
#define GFX_0009AF_16X13_1BPP_H

#define GFX_0009AF_16X13_1BPP_W       16
#define GFX_0009AF_16X13_1BPP_H_PX    13
#define GFX_0009AF_16X13_1BPP_BPP     1
#define GFX_0009AF_16X13_1BPP_PALETTE 1
#define GFX_0009AF_16X13_1BPP_STRIDE  2
#define GFX_0009AF_16X13_1BPP_OFFSET  0x0009af

static const unsigned char gfx_0009af_16x13_1bpp_data[36] = {
    0x10, 0x0d, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x03, 0x00, 0x07, 0x00, 0x0e, 0x00,
    0x1c, 0x00, 0x38, 0x00, 0x7f, 0xfc, 0xff, 0xfc, 0x7f, 0xfc, 0x38, 0x00, 0x1c, 0x00, 0x0e, 0x00,
    0x07, 0x00, 0x03, 0x00,
};

#endif
