/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x000967 (flash 0x04b18f), 38 bytes, 16 x 14 at 1 bpp with a
 * 1-entry palette. Provenance: run after 0x000937.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         14 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_000967_16X14_1BPP_H
#define GFX_000967_16X14_1BPP_H

#define GFX_000967_16X14_1BPP_W       16
#define GFX_000967_16X14_1BPP_H_PX    14
#define GFX_000967_16X14_1BPP_BPP     1
#define GFX_000967_16X14_1BPP_PALETTE 1
#define GFX_000967_16X14_1BPP_STRIDE  2
#define GFX_000967_16X14_1BPP_OFFSET  0x000967

static const unsigned char gfx_000967_16x14_1bpp_data[38] = {
    0x10, 0x0e, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf8, 0x8a, 0x7f, 0xf8, 0xff, 0xfc, 0xff, 0xfc,
    0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc,
    0xff, 0xfc, 0xff, 0xfc, 0x7f, 0xf8,
};

#endif
