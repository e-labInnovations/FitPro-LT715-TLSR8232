/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x00098d (flash 0x04b1b5), 34 bytes, 16 x 12 at 1 bpp with a
 * 1-entry palette. Provenance: direct: DAT_00008e4c.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         12 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_00098D_16X12_1BPP_H
#define GFX_00098D_16X12_1BPP_H

#define GFX_00098D_16X12_1BPP_W       16
#define GFX_00098D_16X12_1BPP_H_PX    12
#define GFX_00098D_16X12_1BPP_BPP     1
#define GFX_00098D_16X12_1BPP_PALETTE 1
#define GFX_00098D_16X12_1BPP_STRIDE  2
#define GFX_00098D_16X12_1BPP_OFFSET  0x00098d

static const unsigned char gfx_00098d_16x12_1bpp_data[34] = {
    0x10, 0x0c, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x81, 0x01, 0x83, 0x07, 0x8f,
    0x0f, 0x9f, 0x3f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x3f, 0xff, 0x0f, 0x9f, 0x07, 0x8f, 0x01, 0x83,
    0x00, 0x81,
};

#endif
