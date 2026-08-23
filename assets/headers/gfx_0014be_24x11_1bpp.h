/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x0014be (flash 0x04bce6), 43 bytes, 24 x 11 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x001448.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         11 rows of 3 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_0014BE_24X11_1BPP_H
#define GFX_0014BE_24X11_1BPP_H

#define GFX_0014BE_24X11_1BPP_W       24
#define GFX_0014BE_24X11_1BPP_H_PX    11
#define GFX_0014BE_24X11_1BPP_BPP     1
#define GFX_0014BE_24X11_1BPP_PALETTE 1
#define GFX_0014BE_24X11_1BPP_STRIDE  3
#define GFX_0014BE_24X11_1BPP_OFFSET  0x0014be

static const unsigned char gfx_0014be_24x11_1bpp_data[43] = {
    0x18, 0x0b, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x3f, 0xff, 0xe0, 0x40, 0x00, 0x10,
    0x80, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x0c, 0x80, 0x00, 0x0c, 0x80, 0x00, 0x0c, 0x80,
    0x00, 0x08, 0x80, 0x00, 0x08, 0x40, 0x00, 0x10, 0x3f, 0xff, 0xe0,
};

#endif
