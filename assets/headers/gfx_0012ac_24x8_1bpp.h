/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x0012ac (flash 0x04bad4), 34 bytes, 24 x 8 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001190.
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

#ifndef GFX_0012AC_24X8_1BPP_H
#define GFX_0012AC_24X8_1BPP_H

#define GFX_0012AC_24X8_1BPP_W       24
#define GFX_0012AC_24X8_1BPP_H_PX    8
#define GFX_0012AC_24X8_1BPP_BPP     1
#define GFX_0012AC_24X8_1BPP_PALETTE 1
#define GFX_0012AC_24X8_1BPP_STRIDE  3
#define GFX_0012AC_24X8_1BPP_OFFSET  0x0012ac

static const unsigned char gfx_0012ac_24x8_1bpp_data[34] = {
    0x18, 0x08, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x82, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x82, 0x71, 0xc0, 0x82, 0x8a, 0x20, 0x82, 0x8b, 0xe0, 0x82, 0x8a, 0x00, 0xfa,
    0x89, 0xc0,
};

#endif
