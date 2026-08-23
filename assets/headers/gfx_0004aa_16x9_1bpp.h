/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x0004aa (flash 0x04acd2), 28 bytes, 16 x 9 at 1 bpp with a
 * 1-entry palette. Provenance: direct: DAT_0000831c.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         9 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_0004AA_16X9_1BPP_H
#define GFX_0004AA_16X9_1BPP_H

#define GFX_0004AA_16X9_1BPP_W       16
#define GFX_0004AA_16X9_1BPP_H_PX    9
#define GFX_0004AA_16X9_1BPP_BPP     1
#define GFX_0004AA_16X9_1BPP_PALETTE 1
#define GFX_0004AA_16X9_1BPP_STRIDE  2
#define GFX_0004AA_16X9_1BPP_OFFSET  0x0004aa

static const unsigned char gfx_0004aa_16x9_1bpp_data[28] = {
    0x10, 0x09, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x9e, 0xc5, 0xe3, 0xb0, 0x92, 0x48, 0x92, 0x48,
    0x92, 0x48, 0x92, 0x48, 0x92, 0x48, 0xe2, 0x48, 0x80, 0x00, 0x80, 0x00,
};

#endif
