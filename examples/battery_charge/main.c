/*
 * Battery gauge and charger detection, as the stock firmware does it
 *
 * Everything here comes out of the decompiled stock image, so this example is
 * really a test of whether the reverse engineering was right:
 *
 *   BATT   the pack voltage on PB1, median-of-16 filtered
 *   PCT    percent through the stock five-step gauge (lib/battery)
 *   CHG    the PB2 charger rail in mV - the pin we had written off as unused
 *   STATE  CHARGING when PB2 sits in the stock window, 4400..6500 mV
 *
 * How to read it:
 *   - unplugged, CHG should sit near zero and STATE should say BATTERY
 *   - on the charger, CHG should jump to somewhere near 5000 and STATE should
 *     flip to CHARGING within a second
 *
 * If CHG stays at zero with the charger plugged in, then PB2 is not the charger
 * sense line on this board revision, and the firmware evidence for it (channel
 * 0x104 selected by FUN_0000c37c) applies to a different revision.
 *
 * PCT is deliberately coarse. The stock gauge only ever reports 0, 25, 50, 75
 * or 100, and reports 100 for everything from 3800 mV up - that flat top is in
 * the firmware's own table, not a bug here.
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

#define Y_TITLE   2
#define Y_BATT   20
#define Y_PCT    34
#define Y_CHG    52
#define Y_STATE  66
#define Y_LOW    84

_attribute_ram_code_ void irq_handler(void) {}

static void draw(int16_t y, const char *s, uint16_t color) {
    gfx_set_text_color_bg(color, ST77XX_BLACK);
    gfx_set_cursor(2, y);
    gfx_print(s);
}

// "BATT 4012mV" - fixed width so the row never leaves stale pixels behind
static void fmt_mv(char *out, const char *tag, unsigned int mv) {
    int i = 0;
    while (*tag) out[i++] = *tag++;
    out[i++] = ' ';
    if (mv > 9999) mv = 9999;
    out[i++] = (char)('0' + (mv / 1000) % 10);
    out[i++] = (char)('0' + (mv / 100) % 10);
    out[i++] = (char)('0' + (mv / 10) % 10);
    out[i++] = (char)('0' + mv % 10);
    out[i++] = 'm';
    out[i++] = 'V';
    out[i] = '\0';
}

// "PCT  75%"
static void fmt_pct(char *out, unsigned int pct) {
    out[0] = 'P'; out[1] = 'C'; out[2] = 'T'; out[3] = ' '; out[4] = ' ';
    if (pct > 100) pct = 100;
    out[5] = pct >= 100 ? '1' : ' ';
    out[6] = pct >= 100 ? '0' : (char)('0' + (pct / 10) % 10);
    out[7] = pct >= 100 ? '0' : (char)('0' + pct % 10);
    out[8] = '%';
    out[9] = '\0';
}

int main() {
    char buf[16];

    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    battery_init();

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_set_font(&FreeMono9pt7b);
    gfx_fill_screen(ST77XX_BLACK);

    draw(Y_TITLE, "BATT + CHG", ST77XX_CYAN);

    while (1) {
        unsigned int batt = battery_read_mv();
        unsigned int chg  = charger_read_mv();
        unsigned int pct  = battery_percent(batt);
        uint8_t      on   = (chg >= CHARGER_PRESENT_MIN_MV
                             && chg <= CHARGER_PRESENT_MAX_MV);

        fmt_mv(buf, "BATT", batt);
        draw(Y_BATT, buf, ST77XX_WHITE);

        fmt_pct(buf, pct);
        draw(Y_PCT, buf, pct <= 25 ? ST77XX_RED : ST77XX_GREEN);

        fmt_mv(buf, "CHG ", chg);
        draw(Y_CHG, buf, ST77XX_WHITE);

        draw(Y_STATE, on ? "CHARGING" : "BATTERY ",
             on ? ST77XX_GREEN : ST77XX_YELLOW);

        draw(Y_LOW, battery_is_low(batt) ? "LOW BATT" : "        ", ST77XX_RED);

        sleep_ms(500);
    }

    return 0;
}
