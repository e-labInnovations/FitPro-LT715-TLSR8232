/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x000462 (flash 0x04ac8a), 40 bytes, 24 x 10 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x0003a6.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         10 rows of 3 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_000462_24X10_1BPP_H
#define GFX_000462_24X10_1BPP_H

#define GFX_000462_24X10_1BPP_W       24
#define GFX_000462_24X10_1BPP_H_PX    10
#define GFX_000462_24X10_1BPP_BPP     1
#define GFX_000462_24X10_1BPP_PALETTE 1
#define GFX_000462_24X10_1BPP_STRIDE  3
#define GFX_000462_24X10_1BPP_OFFSET  0x000462

static const unsigned char gfx_000462_24x10_1bpp_data[40] = {
    0x18, 0x0a, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x9e, 0xc5, 0x7f, 0xff, 0xe0, 0x80, 0x00, 0x10,
    0xbf, 0xff, 0xd0, 0xbf, 0xff, 0xd8, 0xbf, 0xff, 0xd8, 0xbf, 0xff, 0xd8, 0xbf, 0xff, 0xd8, 0xbf,
    0xff, 0xd0, 0x80, 0x00, 0x10, 0x7f, 0xff, 0xe0,
};

#endif
