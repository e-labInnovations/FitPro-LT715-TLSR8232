/*
 * Touch key driver demo — FitPro LT715 / TLSR8232
 *
 * Uses lib/touch (PC2, active high, level-following) as a real UI button:
 *
 *   tap         -> counter T, screen flashes the event name
 *   double tap  -> counter D
 *   long press  -> counter L, and toggles the backlight (screen sleep/wake)
 *
 * A long press never also counts as a tap, and the hold time is shown live
 * while the finger is down.
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
#include "../../lib/fonts/FreeSans12pt7b.h"
// Per-app timing override — must be #define-d before the header. Library
// defaults: TOUCH_LONG_MS 1400, TOUCH_DTAP_MS 500, TOUCH_DEBOUNCE_MS 24.
// #define TOUCH_LONG_MS 1000
#include "../../lib/touch/touch.h"

// UART logging on PB4/PB5 — one line per event.
#define ENABLE_UART   1

#define POLL_MS       5
#define HOLD_REDRAW   100

#define Y_TITLE       2
#define Y_EVENT       18
#define H_EVENT       26
#define Y_COUNT       52
#define Y_LONG        64
#define Y_HOLD        78
#define Y_STATE       96
#define H_STATE       26

#if ENABLE_UART
#include "../../lib/uart/uart_helper.h"

static uart_data_t tx;
static uint8_t     rx_buf[32];
static char        ubuf[96];
static unsigned    ulen;

static void us(const char *s) {
    while (*s && ulen < sizeof(ubuf)) ubuf[ulen++] = *s++;
}

static void un(unsigned int v) {
    char t[12];
    int  i = 0;
    if (v == 0) { us("0"); return; }
    while (v > 0) { t[i++] = '0' + (v % 10); v /= 10; }
    while (i > 0) { i--; if (ulen < sizeof(ubuf)) ubuf[ulen++] = t[i]; }
}

static void uflush(void) {
    unsigned int i;
    for (i = 0; i < ulen; i++) tx.data[i] = (uint8_t)ubuf[i];
    tx.len = ulen;
    uart_send(&tx);
    ulen = 0;
    sleep_ms(8);  // DMA send is asynchronous — let it drain before refilling
}
#endif

_attribute_ram_code_ void irq_handler(void) {}

static void draw_mono(int16_t y, const char *s, uint16_t color) {
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color_bg(color, ST77XX_BLACK);
    gfx_set_cursor(2, y);
    gfx_print(s);
}

static void draw_event(const char *s, uint16_t color) {
    gfx_fill_rect(0, Y_EVENT, ST7735_TFTWIDTH, H_EVENT, ST77XX_BLACK);
    gfx_set_font(&FreeSans12pt7b);
    gfx_set_text_color(color);
    gfx_set_cursor(30, Y_EVENT + 3);
    gfx_print(s);
}

static void draw_state(uint8_t down) {
    gfx_fill_rect(0, Y_STATE, ST7735_TFTWIDTH, H_STATE,
                  down ? ST77XX_GREEN : ST77XX_BLACK);
    gfx_set_font(&FreeSans12pt7b);
    gfx_set_text_color(down ? ST77XX_BLACK : ST77XX_WHITE);
    gfx_set_cursor(down ? 28 : 40, Y_STATE + 3);
    gfx_print(down ? "TOUCH" : "----");
}

// "T=003 D=001"
static void fmt_counts(char *out, unsigned int t, unsigned int d) {
    if (t > 999) t = 999;
    if (d > 999) d = 999;
    out[0]  = 'T'; out[1] = '=';
    out[2]  = (char)('0' + (t / 100) % 10);
    out[3]  = (char)('0' + (t / 10) % 10);
    out[4]  = (char)('0' + t % 10);
    out[5]  = ' ';
    out[6]  = 'D'; out[7] = '=';
    out[8]  = (char)('0' + (d / 100) % 10);
    out[9]  = (char)('0' + (d / 10) % 10);
    out[10] = (char)('0' + d % 10);
    out[11] = '\0';
}

// "L=002 bl:on"
static void fmt_long(char *out, unsigned int l, uint8_t bl) {
    if (l > 999) l = 999;
    out[0] = 'L'; out[1] = '=';
    out[2] = (char)('0' + (l / 100) % 10);
    out[3] = (char)('0' + (l / 10) % 10);
    out[4] = (char)('0' + l % 10);
    out[5] = ' '; out[6] = 'b'; out[7] = 'l'; out[8] = ':';
    out[9]  = bl ? '1' : '0';
    out[10] = ' ';
    out[11] = '\0';
}

// "hold 0345ms"
static void fmt_hold(char *out, unsigned int ms) {
    if (ms > 9999) ms = 9999;
    out[0] = 'h'; out[1] = 'o'; out[2] = 'l'; out[3] = 'd'; out[4] = ' ';
    out[5] = (char)('0' + (ms / 1000) % 10);
    out[6] = (char)('0' + (ms / 100) % 10);
    out[7] = (char)('0' + (ms / 10) % 10);
    out[8] = (char)('0' + ms % 10);
    out[9]  = 'm'; out[10] = 's';
    out[11] = '\0';
}

int main() {
    char         buf[12];
    unsigned int taps = 0, dtaps = 0, longs = 0;
    uint8_t      bl = 1;
    unsigned int hold_t0;

    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

#if ENABLE_UART
    uart_helper_init(rx_buf, sizeof(rx_buf));
#endif

    touch_init();

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_fill_screen(ST77XX_BLACK);

    draw_mono(Y_TITLE, "TOUCH DEMO", ST77XX_CYAN);
    draw_event("----", ST77XX_WHITE);
    fmt_counts(buf, 0, 0);
    draw_mono(Y_COUNT, buf, ST77XX_WHITE);
    fmt_long(buf, 0, bl);
    draw_mono(Y_LONG, buf, ST77XX_WHITE);
    fmt_hold(buf, 0);
    draw_mono(Y_HOLD, buf, ST77XX_WHITE);
    draw_state(0);

    hold_t0 = clock_time();

    while (1) {
        touch_event_t ev = touch_poll();

        switch (ev) {
            case TOUCH_DOWN:
                draw_state(1);
                draw_event("DOWN", ST77XX_YELLOW);
                break;

            case TOUCH_TAP:
                taps++;
                draw_state(0);
                draw_event("TAP", ST77XX_YELLOW);
                fmt_counts(buf, taps, dtaps);
                draw_mono(Y_COUNT, buf, ST77XX_YELLOW);
                break;

            case TOUCH_DOUBLE_TAP:
                dtaps++;
                draw_state(0);
                draw_event("DTAP", ST77XX_MAGENTA);
                fmt_counts(buf, taps, dtaps);
                draw_mono(Y_COUNT, buf, ST77XX_MAGENTA);
                break;

            case TOUCH_LONG_PRESS:
                longs++;
                bl = bl ? 0 : 1;
                if (bl) backlight_on();
                else    backlight_off();
                draw_event("LONG", ST77XX_GREEN);
                fmt_long(buf, longs, bl);
                draw_mono(Y_LONG, buf, ST77XX_GREEN);
                break;

            case TOUCH_UP:
                draw_state(0);
                draw_event("UP", ST77XX_WHITE);
                break;

            default:
                break;
        }

#if ENABLE_UART
        if (ev != TOUCH_NONE) {
            us(touch_event_name(ev));
            us(" ms=");
            un(touch_press_ms());
            us("\n");
            uflush();
        }
#endif

        if (clock_time_exceed(hold_t0, HOLD_REDRAW * 1000)) {
            hold_t0 = clock_time();
            fmt_hold(buf, touch_press_ms());
            draw_mono(Y_HOLD, buf, touch_is_down() ? ST77XX_GREEN : ST77XX_WHITE);
        }

        sleep_ms(POLL_MS);
    }

    return 0;
}
