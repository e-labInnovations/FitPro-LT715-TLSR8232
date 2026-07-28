// Color mapping diagnostic.
//
// Left half of the screen is painted with display_fill_window() — the path the
// working display example uses. Right half is painted with
// display_draw_image_rle() — the image path. Both halves use the SAME three
// raw 16-bit words, so:
//
//   * left and right differ   -> the bug is in the image/blit path
//   * left and right match    -> the bug is the panel's channel mapping, and
//                                the bar order tells us what it actually is
//
// Bars, top to bottom, are the three RGB565 primaries:
//
//   bar 1  0xF800  bits [15:11]   should be RED
//   bar 2  0x07E0  bits [10:5]    should be GREEN
//   bar 3  0x001F  bits [4:0]     should be BLUE
//
// Report what you actually see, top to bottom.

#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/timer.h"
#include "../../lib/display/display.h"

#define BAR_H  42
#define HALF_W 64

_attribute_ram_code_ void irq_handler(void) {}

static const uint16_t bar_color[3] = {0xF800, 0x07E0, 0x001F};

// One 64x126 RLE image: three solid bands, (pixel_count, color) pairs.
static const uint16_t bars_rle[6] = {
    HALF_W * BAR_H, 0xF800,
    HALF_W * BAR_H, 0x07E0,
    HALF_W * BAR_H, 0x001F,
};

int main() {
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    display_init(INITR_GREENTAB, 0);
    display_fill_screen(0x0000);

    // Left: the proven fill path
    for (uint8_t i = 0; i < 3; i++) {
        display_fill_window(0, i * BAR_H, HALF_W - 2, (i + 1) * BAR_H - 1, bar_color[i]);
    }

    // Right: the image path, same words
    display_draw_image_rle(HALF_W, 0, HALF_W, BAR_H * 3, bars_rle, 3);

    while (1);
    return 0;
}
