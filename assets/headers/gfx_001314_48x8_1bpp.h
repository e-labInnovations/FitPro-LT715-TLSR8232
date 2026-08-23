/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x001314 (flash 0x04bb3c), 58 bytes, 48 x 8 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001190.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         8 rows of 6 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_001314_48X8_1BPP_H
#define GFX_001314_48X8_1BPP_H

#define GFX_001314_48X8_1BPP_W       48
#define GFX_001314_48X8_1BPP_H_PX    8
#define GFX_001314_48X8_1BPP_BPP     1
#define GFX_001314_48X8_1BPP_PALETTE 1
#define GFX_001314_48X8_1BPP_STRIDE  6
#define GFX_001314_48X8_1BPP_OFFSET  0x001314

static const unsigned char gfx_001314_48x8_1bpp_data[58] = {
    0x30, 0x08, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0x00, 0x01, 0x00, 0x00, 0x80, 0x80, 0x00, 0x01, 0x00, 0x00, 0x80, 0x83, 0x8e, 0x39, 0xc7,
    0x1c, 0xa0, 0xf0, 0x51, 0x45, 0x28, 0xa2, 0xc0, 0x83, 0xd0, 0x7d, 0x28, 0xa2, 0xc0, 0x84, 0x51,
    0x41, 0x28, 0xa2, 0xa0, 0x83, 0xce, 0x39, 0xc7, 0x1c, 0x90,
};

#endif
