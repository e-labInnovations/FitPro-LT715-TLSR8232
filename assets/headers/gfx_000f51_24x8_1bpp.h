/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x000f51 (flash 0x04b779), 34 bytes, 24 x 8 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x000d4c.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         8 rows of 3 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_000F51_24X8_1BPP_H
#define GFX_000F51_24X8_1BPP_H

#define GFX_000F51_24X8_1BPP_W       24
#define GFX_000F51_24X8_1BPP_H_PX    8
#define GFX_000F51_24X8_1BPP_BPP     1
#define GFX_000F51_24X8_1BPP_PALETTE 1
#define GFX_000F51_24X8_1BPP_STRIDE  3
#define GFX_000F51_24X8_1BPP_OFFSET  0x000f51

static const unsigned char gfx_000f51_24x8_1bpp_data[34] = {
    0x18, 0x08, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0x01, 0x00, 0x80, 0x01, 0x00,
    0x90, 0x01, 0x00, 0xa3, 0x19, 0x00, 0xc4, 0xa5, 0x00, 0xc4, 0x1d, 0x00, 0xa4, 0xa5, 0x00, 0x93,
    0x1d, 0x00,
};

#endif
