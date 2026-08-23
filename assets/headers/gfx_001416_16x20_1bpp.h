/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x001416 (flash 0x04bc3e), 50 bytes, 16 x 20 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001190.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         20 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_001416_16X20_1BPP_H
#define GFX_001416_16X20_1BPP_H

#define GFX_001416_16X20_1BPP_W       16
#define GFX_001416_16X20_1BPP_H_PX    20
#define GFX_001416_16X20_1BPP_BPP     1
#define GFX_001416_16X20_1BPP_PALETTE 1
#define GFX_001416_16X20_1BPP_STRIDE  2
#define GFX_001416_16X20_1BPP_OFFSET  0x001416

static const unsigned char gfx_001416_16x20_1bpp_data[50] = {
    0x10, 0x14, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x5c, 0xde, 0x19, 0x80, 0x19, 0x80, 0x19, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x19, 0x80, 0x19, 0x80, 0x19, 0x80, 0x00, 0x00, 0x00, 0x00, 0x19, 0x80,
    0x19, 0x80, 0xf9, 0xf0, 0x79, 0xe0, 0x79, 0xe0, 0x39, 0xc0, 0x39, 0xc0, 0x19, 0x80, 0x19, 0x80,
    0x09, 0x00,
};

#endif
