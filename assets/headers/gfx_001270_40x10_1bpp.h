/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x001270 (flash 0x04ba98), 60 bytes, 40 x 10 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001190.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         10 rows of 5 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_001270_40X10_1BPP_H
#define GFX_001270_40X10_1BPP_H

#define GFX_001270_40X10_1BPP_W       40
#define GFX_001270_40X10_1BPP_H_PX    10
#define GFX_001270_40X10_1BPP_BPP     1
#define GFX_001270_40X10_1BPP_PALETTE 1
#define GFX_001270_40X10_1BPP_STRIDE  5
#define GFX_001270_40X10_1BPP_OFFSET  0x001270

static const unsigned char gfx_001270_40x10_1bpp_data[60] = {
    0x28, 0x0a, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6,
    0x00, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x00, 0x00, 0x00, 0xaa, 0x71, 0xce, 0x71, 0xce, 0xaa, 0x8a,
    0x10, 0x0a, 0x51, 0xaa, 0xf9, 0x8c, 0x7a, 0x5f, 0x92, 0x80, 0x42, 0x8a, 0x50, 0x92, 0x73, 0x9c,
    0x79, 0xce, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x03, 0x80,
};

#endif
