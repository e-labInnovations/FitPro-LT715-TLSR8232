/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x001222 (flash 0x04ba4a), 50 bytes, 40 x 8 at 1 bpp with a
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

#ifndef GFX_001222_40X8_1BPP_H
#define GFX_001222_40X8_1BPP_H

#define GFX_001222_40X8_1BPP_W       40
#define GFX_001222_40X8_1BPP_H_PX    8
#define GFX_001222_40X8_1BPP_BPP     1
#define GFX_001222_40X8_1BPP_PALETTE 1
#define GFX_001222_40X8_1BPP_STRIDE  5
#define GFX_001222_40X8_1BPP_OFFSET  0x001222

static const unsigned char gfx_001222_40x8_1bpp_data[50] = {
    0x28, 0x08, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8,
    0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x44, 0x00, 0x00, 0x22, 0x4a, 0xee, 0x72, 0x80, 0x22, 0x4a,
    0x44, 0x8b, 0x00, 0x22, 0xaa, 0x44, 0xfa, 0x00, 0x22, 0xaa, 0x44, 0x82, 0x00, 0x21, 0x12, 0x22,
    0x72, 0x00,
};

#endif
