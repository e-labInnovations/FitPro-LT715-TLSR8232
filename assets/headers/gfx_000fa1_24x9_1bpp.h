/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x000fa1 (flash 0x04b7c9), 37 bytes, 24 x 9 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x000d4c.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         9 rows of 3 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_000FA1_24X9_1BPP_H
#define GFX_000FA1_24X9_1BPP_H

#define GFX_000FA1_24X9_1BPP_W       24
#define GFX_000FA1_24X9_1BPP_H_PX    9
#define GFX_000FA1_24X9_1BPP_BPP     1
#define GFX_000FA1_24X9_1BPP_PALETTE 1
#define GFX_000FA1_24X9_1BPP_STRIDE  3
#define GFX_000FA1_24X9_1BPP_OFFSET  0x000fa1

static const unsigned char gfx_000fa1_24x9_1bpp_data[37] = {
    0x18, 0x09, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x70, 0x00, 0x60, 0x88, 0x00, 0x90,
    0x83, 0x8c, 0x90, 0x72, 0x52, 0x10, 0x0a, 0x52, 0x20, 0x8a, 0x52, 0x40, 0x73, 0x8c, 0xf0, 0x02,
    0x00, 0x00, 0x02, 0x00, 0x00,
};

#endif
