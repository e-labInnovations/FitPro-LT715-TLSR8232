/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x0011f0 (flash 0x04ba18), 50 bytes, 40 x 8 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001190.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         8 rows of 5 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_0011F0_40X8_1BPP_H
#define GFX_0011F0_40X8_1BPP_H

#define GFX_0011F0_40X8_1BPP_W       40
#define GFX_0011F0_40X8_1BPP_H_PX    8
#define GFX_0011F0_40X8_1BPP_BPP     1
#define GFX_0011F0_40X8_1BPP_PALETTE 1
#define GFX_0011F0_40X8_1BPP_STRIDE  5
#define GFX_0011F0_40X8_1BPP_OFFSET  0x0011f0

static const unsigned char gfx_0011f0_40x8_1bpp_data[50] = {
    0x28, 0x08, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x01, 0x00, 0x00, 0x88,
    0x80, 0x39, 0x00, 0x00, 0x94, 0x80, 0x45, 0x00, 0x08, 0x94, 0x9c, 0x81, 0xe3, 0x9c, 0x55, 0x22,
    0x81, 0x10, 0x48, 0x55, 0x3e, 0x81, 0x13, 0xc8, 0x22, 0x20, 0x45, 0x14, 0x48, 0x22, 0x1c, 0x39,
    0x13, 0xc4,
};

#endif
