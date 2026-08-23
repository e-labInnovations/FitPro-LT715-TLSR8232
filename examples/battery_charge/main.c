/*
 * Battery gauge and charger detection, as the stock firmware does it
 *
 * Everything here comes out of the decompiled stock image, so this example is
 * really a test of whether the reverse engineering was right:
 *
 *   BATT   pack voltage on PB1, median-of-16 filtered
 *   PCT    percent through the stock five-step gauge (lib/battery)
 *   CHG    the charger rail in mV - the PB2 pin reading scaled back up through
 *          the board's 1:8 divider, so it is in the same units as the stock
 *          firmware's own thresholds
 *   PIN    the raw PB2 pin voltage behind that, for calibration
 *   STATE  CHARGING when CHG sits inside the stock window, 4400..6500 mV
 *
 * What the numbers do, measured on hardware:
 *
 *   unplugged   CHG tracks the battery, because the sense node follows whichever
 *               of VBAT and VBUS is higher. 4092 mV of pack read 4120 mV here.
 *   charging    CHG jumps to about 5200, a USB supply.
 *
 * So CHG is NOT zero when unplugged, and testing it against zero would report
 * charging forever. The window's lower bound of 4400 mV is what separates the
 * two cases, and it works precisely because 4400 is above a full battery.
 *
 * PCT is deliberately coarse: the stock gauge only reports 0, 25, 50, 75 or 100,
 * and reports 100 for everything from 3800 mV up. That flat top is in the
 * firmware's own table, not a rounding artefact here.
 *
 * Layout: FreeMono9pt7b is monospaced at an 11 px advance, so a 128 px wide
 * panel holds exactly 11 characters. Labelled rows are 5 + 6 = 11 and fit;
 * the status lines are printed on their own, unlabelled, because "CHARGING"
 * behind a padded label came to 13 and wrapped onto the row below - which is
 * what left "RY" and a stray "TT" on screen. Wrapping is off as a backstop.
 *
 * Values are right-aligned in fixed-width fields and drawn opaque, so a
 * shrinking number cannot leave a stale digit behind.
 */

#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"

#include "../../lib/battery/battery.h"
#include "../../lib/display/display.h"
#include "../../lib/display/gfx.h"
#include "../../lib/fonts/FreeMono9pt7b.h"

#define ROW0      2      // title
#define ROW_H    15
#define LABEL_W   5      // "BATT " - padded so values start in one column
#define VALUE_W   6      // right-aligned, widest is "5224mV"
#define LINE_W   11      // 128 px / 11 px per character - the hard limit

// Catch a layout edit that would overflow the panel at build time instead of
// on screen, which is how the wrapped status rows got shipped once already.
#if LABEL_W + VALUE_W > LINE_W
#error "labelled row is wider than the panel"
#endif

_attribute_ram_code_ void irq_handler(void) {}

static void draw(int16_t y, const char *s, uint16_t color) {
    gfx_set_text_color_bg(color, ST77XX_BLACK);
    gfx_set_cursor(2, y);
    gfx_print(s);
}

// label padded to LABEL_W, then text right-aligned in VALUE_W
static void fmt_row(char *out, const char *label, const char *value) {
    int i = 0, n = 0, pad;

    while (label[n]) n++;
    while (i < LABEL_W) out[i] = (i < n) ? label[i] : ' ', i++;

    for (n = 0; value[n]; n++)
        ;
    for (pad = VALUE_W - n; pad > 0; pad--) out[i++] = ' ';
    for (n = 0; value[n]; n++) out[i++] = value[n];
    out[i] = '\0';
}

// unsigned decimal, no leading zeros - "515", "5224"
static void fmt_uint(char *out, unsigned int v) {
    char tmp[8];
    int  i = 0, j = 0;

    if (v == 0) tmp[i++] = '0';
    while (v > 0) tmp[i++] = (char)('0' + v % 10), v /= 10;
    while (i > 0) out[j++] = tmp[--i];
    out[j] = '\0';
}

static void fmt_mv(char *out, unsigned int mv) {
    int i;
    fmt_uint(out, mv);
    for (i = 0; out[i]; i++)
        ;
    out[i++] = 'm';
    out[i++] = 'V';
    out[i] = '\0';
}

static void fmt_pct(char *out, unsigned int pct) {
    int i;
    fmt_uint(out, pct);
    for (i = 0; out[i]; i++)
        ;
    out[i++] = '%';
    out[i] = '\0';
}

int main() {
    char row[24], val[12];

    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    battery_init();

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_set_font(&FreeMono9pt7b);
    // No wrapping: an over-long row must be visibly clipped, not silently
    // continued on top of the next one.
    gfx_set_text_wrap(false);
    gfx_fill_screen(ST77XX_BLACK);

    draw(ROW0, "BATT + CHG", ST77XX_CYAN);

    while (1) {
        unsigned int batt   = battery_read_mv();
        unsigned int pin_mv = charger_read_pin_mv();
        unsigned int rail   = pin_mv * CHARGER_DIVIDER;
        unsigned int pct    = battery_percent(batt);
        uint8_t      on     = (rail >= CHARGER_PRESENT_MIN_MV
                               && rail <= CHARGER_PRESENT_MAX_MV);

        fmt_mv(val, batt);
        fmt_row(row, "BATT", val);
        draw(ROW0 + ROW_H * 2, row, ST77XX_WHITE);

        fmt_pct(val, pct);
        fmt_row(row, "PCT", val);
        draw(ROW0 + ROW_H * 3, row, pct <= 25 ? ST77XX_RED : ST77XX_GREEN);

        fmt_mv(val, rail);
        fmt_row(row, "CHG", val);
        draw(ROW0 + ROW_H * 4, row, on ? ST77XX_GREEN : ST77XX_WHITE);

        fmt_mv(val, pin_mv);
        fmt_row(row, "PIN", val);
        draw(ROW0 + ROW_H * 5, row, ST77XX_BLUE);

        draw(ROW0 + ROW_H * 6, on ? "CHARGING" : " BATTERY",
             on ? ST77XX_GREEN : ST77XX_YELLOW);

        // Stock behaviour: the low-battery warning is gated on NOT charging
        // (FUN_0000be6c checks both), so plugging in clears it immediately
        // rather than waiting for the pack to recover.
        draw(ROW0 + ROW_H * 7,
             (battery_is_low(batt) && !on) ? "LOW BATT" : "        ",
             ST77XX_RED);

        sleep_ms(500);
    }

    return 0;
}
