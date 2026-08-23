/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x000937 (flash 0x04b15f), 48 bytes, 16 x 19 at 1 bpp with a
 * 1-entry palette. Provenance: direct: DAT_00008e54.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         19 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_000937_16X19_1BPP_H
#define GFX_000937_16X19_1BPP_H

#define GFX_000937_16X19_1BPP_W       16
#define GFX_000937_16X19_1BPP_H_PX    19
#define GFX_000937_16X19_1BPP_BPP     1
#define GFX_000937_16X19_1BPP_PALETTE 1
#define GFX_000937_16X19_1BPP_STRIDE  2
#define GFX_000937_16X19_1BPP_OFFSET  0x000937

static const unsigned char gfx_000937_16x19_1bpp_data[48] = {
    0x10, 0x13, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf8, 0x8a, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c,
    0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c,
    0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c, 0xf8, 0x7c,
};

#endif
