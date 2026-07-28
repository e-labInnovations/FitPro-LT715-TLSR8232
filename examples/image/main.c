// Color image example — draws an RLE-compressed 128x128 background, then a
// 32x32 sprite twice: once opaque (fast window blit) and once with a 1bpp
// alpha mask so the background shows through around it.
//
// Both headers are generated from PNGs by tools/img2c.py — see README.md.

#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/timer.h"
#include "../../lib/display/display.h"
#include "../../lib/display/gfx.h"
#include "../../lib/fonts/FreeSans12pt7b.h"

#include "sunset.h"   // sunset_rle[]  — 128x128, 860 bytes
#include "heart.h"    // heart_data[]  — 32x32 BGR565 + heart_mask[] 1bpp

_attribute_ram_code_ void irq_handler(void) {}

int main() {
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);

    // 1. Full-screen background straight from RLE — no framebuffer, no RAM copy.
    display_draw_image_rle(0, 0, SUNSET_WIDTH, SUNSET_HEIGHT,
                           sunset_rle, SUNSET_RLE_RUNS);

    // 2. Opaque blit: every pixel of the sprite is written, including the
    //    black that the transparent pixels were flattened onto.
    display_draw_image(6, 6, HEART_WIDTH, HEART_HEIGHT, heart_data);

    // 3. Masked blit: only pixels whose mask bit is set are drawn, so the
    //    sprite sits on the background with no black box around it.
    gfx_draw_rgb_bitmap_with_mask(48, 74, heart_data, heart_mask,
                                  HEART_WIDTH, HEART_HEIGHT);

    // Label
    gfx_set_font(&FreeSans12pt7b);
    gfx_set_text_color(ST77XX_WHITE);
    gfx_set_cursor(44, 28);
    gfx_print("BGR565");

    while (1);
    return 0;
}
