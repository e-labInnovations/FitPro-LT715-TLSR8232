// e-lab innovations boot splash for the FitPro LT715.
//
//   frame 1: square logo, centered, for 3 seconds
//   frame 2: wide logo + www.elabins.com, held forever
//
// All three images are RLE color bitmaps in flash (~8.3 KB total) blitted
// straight to the panel — see README.md for how they were generated.

#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/timer.h"
#include "../../lib/display/display.h"

#include "elabins_icon.h"   // 96x96 square mark
#include "elabins_wide.h"   // 124x38 wordmark
#include "elabins_url.h"    // 76x7   www.elabins.com

#define SPLASH_MS 3000

// Center an image of width w on the 128px panel
#define CENTER_X(w) ((ST7735_TFTWIDTH - (w)) / 2)

_attribute_ram_code_ void irq_handler(void) {}

static void draw_icon_frame(void) {
    display_fill_screen(ST77XX_BLACK);
    display_draw_image_rle(CENTER_X(ELABINS_ICON_WIDTH), 16,
                           ELABINS_ICON_WIDTH, ELABINS_ICON_HEIGHT,
                           elabins_icon_rle, ELABINS_ICON_RLE_RUNS);
}

static void draw_wordmark_frame(void) {
    display_fill_screen(ST77XX_BLACK);
    display_draw_image_rle(CENTER_X(ELABINS_WIDE_WIDTH), 34,
                           ELABINS_WIDE_WIDTH, ELABINS_WIDE_HEIGHT,
                           elabins_wide_rle, ELABINS_WIDE_RLE_RUNS);
    display_draw_image_rle(CENTER_X(ELABINS_URL_WIDTH), 86,
                           ELABINS_URL_WIDTH, ELABINS_URL_HEIGHT,
                           elabins_url_rle, ELABINS_URL_RLE_RUNS);
}

int main() {
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    display_init(INITR_GREENTAB, 0);

    draw_icon_frame();
    sleep_ms(SPLASH_MS);
    draw_wordmark_frame();

    while (1);
    return 0;
}
