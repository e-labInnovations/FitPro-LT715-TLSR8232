/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x00115a (flash 0x04b982), 54 bytes, 16 x 22 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001058.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         22 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_00115A_16X22_1BPP_H
#define GFX_00115A_16X22_1BPP_H

#define GFX_00115A_16X22_1BPP_W       16
#define GFX_00115A_16X22_1BPP_H_PX    22
#define GFX_00115A_16X22_1BPP_BPP     1
#define GFX_00115A_16X22_1BPP_PALETTE 1
#define GFX_00115A_16X22_1BPP_STRIDE  2
#define GFX_00115A_16X22_1BPP_OFFSET  0x00115a

static const unsigned char gfx_00115a_16x22_1bpp_data[54] = {
    0x10, 0x16, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x37, 0x16, 0x00, 0x00, 0xff, 0xe0, 0xff, 0xf8,
    0xff, 0xfc, 0xff, 0xfe, 0xf0, 0x3e, 0xf0, 0x1f, 0xf0, 0x1f, 0xf0, 0x1f, 0xf0, 0x3e, 0xf0, 0x7e,
    0xf1, 0xfc, 0xf1, 0xf8, 0xf1, 0xe0, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00,
    0xf0, 0x00, 0xf0, 0x00, 0x00, 0x00,
};

#endif
