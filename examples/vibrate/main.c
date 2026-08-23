/*
 * Vibrator motor on PA5 — FitPro LT715 / TLSR8232
 *
 * PA5 (package pin 9) came out of decompiling the stock firmware, not out of a
 * pin sweep: see lib/vibrate/vibrate.h. This confirms it on hardware and
 * reproduces the three stock patterns.
 *
 * Tap the touch key to fire the next pattern; it runs them in order and also
 * cycles on its own every few seconds so it works untouched.
 */

#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/timer.h"

#include "../../lib/display/display.h"
#include "../../lib/display/gfx.h"
#include "../../lib/fonts/FreeMono9pt7b.h"
#include "../../lib/touch/touch.h"
#include "../../lib/vibrate/vibrate.h"

#define AUTO_MS   6000
#define PULSE_MS  120

_attribute_ram_code_ void irq_handler(void) {}

typedef struct {
    const char  *name;
    unsigned int pulses;
} pattern_t;

static const pattern_t PATTERNS[] = {
    { "SHORT  x2 ", VIBRATE_SHORT_PULSES },
    { "MEDIUM x4 ", VIBRATE_MEDIUM_PULSES },
    { "LONG   x12", VIBRATE_LONG_PULSES },
};
#define NPAT (int)(sizeof(PATTERNS) / sizeof(PATTERNS[0]))

static void draw(int16_t y, const char *s, uint16_t color) {
    gfx_set_text_color_bg(color, ST77XX_BLACK);
    gfx_set_cursor(2, y);
    gfx_print(s);
}

int main() {
    int          pat = 0;
    unsigned int t0;

    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    vibrate_init();
    touch_init();

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_set_font(&FreeMono9pt7b);
    gfx_fill_screen(ST77XX_BLACK);

    draw(2,  "VIBRATE", ST77XX_CYAN);
    draw(16, "PA5 pin 9", ST77XX_MAGENTA);
    draw(44, "tap = next", ST77XX_WHITE);

    t0 = clock_time();
    while (1) {
        touch_event_t ev = touch_poll();
        int fire = (ev == TOUCH_TAP || ev == TOUCH_DOUBLE_TAP);

        if (clock_time_exceed(t0, AUTO_MS * 1000)) fire = 1;

        if (fire) {
            t0 = clock_time();
            draw(28, PATTERNS[pat].name, ST77XX_GREEN);
            vibrate_pulses(PATTERNS[pat].pulses, PULSE_MS);
            draw(28, PATTERNS[pat].name, ST77XX_WHITE);
            pat = (pat + 1) % NPAT;
        }

        sleep_ms(5);
    }

    return 0;
}
