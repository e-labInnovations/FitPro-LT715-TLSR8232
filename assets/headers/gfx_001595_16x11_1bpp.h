/* Image from the stock LT716 firmware.
 *
 * Asset offset 0x001595 (flash 0x04bdbd), 32 bytes, 16 x 11 at 1 bpp with a
 * 1-entry palette. Provenance: direct: DAT_00007c74.
 *
 * The record is kept whole so a decoder can take it unchanged:
 *
 *   bytes 0..1   width, height
 *   bytes 2..3   bits per pixel
 *   bytes 4..7   palette entry count
 *   bytes 8..    palette, RGB565 little-endian
 *   then         11 rows of 2 bytes, top-down, most significant bit leftmost
 *
 * At 1 bpp, index 0 is transparent and index 1 is the palette colour.
 */

#ifndef GFX_001595_16X11_1BPP_H
#define GFX_001595_16X11_1BPP_H

#define GFX_001595_16X11_1BPP_W       16
#define GFX_001595_16X11_1BPP_H_PX    11
#define GFX_001595_16X11_1BPP_BPP     1
#define GFX_001595_16X11_1BPP_PALETTE 1
#define GFX_001595_16X11_1BPP_STRIDE  2
#define GFX_001595_16X11_1BPP_OFFSET  0x001595

static const unsigned char gfx_001595_16x11_1bpp_data[32] = {
    0x10, 0x0b, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0x20, 0x08, 0x40, 0x04, 0x48, 0x24,
    0x90, 0x12, 0x93, 0x92, 0x93, 0x92, 0x93, 0x92, 0x90, 0x12, 0x48, 0x24, 0x40, 0x04, 0x20, 0x08,
};

#endif
