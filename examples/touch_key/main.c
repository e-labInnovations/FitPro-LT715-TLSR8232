/*
 * Touch key on PC2 (pin 21) — FitPro LT715 / TLSR8232
 *
 * pin_probe found PC2 moving when the watch face is touched. This narrows it to
 * that one pin and answers the questions a driver needs:
 *
 *   - which sense mode works      (pull-up / pull-down / float)
 *   - which level means "touched" (idle level is measured at boot)
 *   - is it a level or a pulse    (hold the key and watch the trace + ms)
 *   - how much bounce is there    (raw trace is drawn undebounced)
 *
 * Screen, top to bottom:
 *   PC2 TOUCH     title
 *   PU F idle1    sense mode / census class / measured idle level
 *   TOUCH / ----  debounced state, green while active
 *   n=012 L=003   presses, of which long (>= LONG_MS)
 *   last 0345ms   duration of the last completed press
 *   trace         raw PC2 level, 1 px per sample, sweeping left to right;
 *                 high = top rail, low = bottom rail
 *
 * A pulse output (touch IC that emits a fixed blip per tap) shows a narrow
 * spike and a `last` value that does not grow when you keep holding. A plain
 * switch or a level output tracks your finger and `last` grows with the hold.
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
#include "../../lib/pinscan/pinscan.h"

// UART logging on PB4/PB5 — one line per completed press.
#define ENABLE_UART   1

#define TOUCH_PIN     GPIO_PC2

// -1 = pick from the census class, else 0=PULLUP 1=PULLDN 2=FLOAT
#define FORCE_MODE    (-1)

#define SAMPLE_MS     8      // one trace pixel per sample: 128 px ~= 1 s sweep
#define DEBOUNCE      3      // consecutive equal samples needed to accept a level
#define LONG_MS       1400  // an ordinary tap on this key already runs 400-550 ms
#define TICKS_PER_MS  16000  // SYS_CLK_16M_Crystal

// Layout
#define Y_TITLE       2
#define Y_MODE        14
#define Y_STATE       28
#define H_STATE       26
#define Y_COUNT       58
#define Y_LAST        70
#define Y_TRACE       84
#define H_TRACE       42
#define TRACE_HI      (Y_TRACE + 3)
#define TRACE_LO      (Y_TRACE + H_TRACE - 4)

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

typedef struct {
    GPIO_PullTypeDef pull;
    const char      *tag;   // 2 chars
} sense_mode_t;

static const sense_mode_t MODES[] = {
    { GPIO_PULL_UP_10K,        "PU" },
    { GPIO_PULL_DOWN_100K,     "PD" },
    { GPIO_PULL_UP_DOWN_FLOAT, "FL" },
};

static void draw_mono(int16_t y, const char *s, uint16_t color) {
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color_bg(color, ST77XX_BLACK);
    gfx_set_cursor(2, y);
    gfx_print(s);
}

static void draw_state(uint8_t active) {
    gfx_fill_rect(0, Y_STATE, ST7735_TFTWIDTH, H_STATE,
                  active ? ST77XX_GREEN : ST77XX_BLACK);
    gfx_set_font(&FreeSans12pt7b);
    gfx_set_text_color(active ? ST77XX_BLACK : ST77XX_WHITE);
    gfx_set_cursor(active ? 28 : 40, Y_STATE + 3);
    gfx_print(active ? "TOUCH" : "----");
}

// "n=012 L=003"
static void fmt_count(char *out, unsigned int n, unsigned int l) {
    if (n > 999) n = 999;
    if (l > 999) l = 999;
    out[0]  = 'n'; out[1] = '=';
    out[2]  = (char)('0' + (n / 100) % 10);
    out[3]  = (char)('0' + (n / 10) % 10);
    out[4]  = (char)('0' + n % 10);
    out[5]  = ' ';
    out[6]  = 'L'; out[7] = '=';
    out[8]  = (char)('0' + (l / 100) % 10);
    out[9]  = (char)('0' + (l / 10) % 10);
    out[10] = (char)('0' + l % 10);
    out[11] = '\0';
}

// "last 0345ms"
static void fmt_last(char *out, unsigned int ms) {
    if (ms > 9999) ms = 9999;
    out[0] = 'l'; out[1] = 'a'; out[2] = 's'; out[3] = 't'; out[4] = ' ';
    out[5] = (char)('0' + (ms / 1000) % 10);
    out[6] = (char)('0' + (ms / 100) % 10);
    out[7] = (char)('0' + (ms / 10) % 10);
    out[8] = (char)('0' + ms % 10);
    out[9] = 'm'; out[10] = 's';
    out[11] = '\0';
}

// "PU F idle1"
static void fmt_mode(char *out, const char *tag, char cls, uint8_t idle) {
    out[0] = tag[0]; out[1] = tag[1];
    out[2] = ' ';
    out[3] = cls;
    out[4] = ' ';
    out[5] = 'i'; out[6] = 'd'; out[7] = 'l'; out[8] = 'e';
    out[9] = (char)('0' + idle);
    out[10] = '\0';
}

int main() {
    char            buf[12];
    pinscan_class_t cls;
    int             mode;
    uint8_t         idle, active_level;

    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

#if ENABLE_UART
    uart_helper_init(rx_buf, sizeof(rx_buf));
#endif

    pinscan_idle(TOUCH_PIN);

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_fill_screen(ST77XX_BLACK);

    cls = pinscan_classify(TOUCH_PIN);

    if (FORCE_MODE >= 0) {
        mode = FORCE_MODE;
    } else if (cls == PINSCAN_FLOAT) {
        // Nothing driving it at rest: a switch or open-drain output. Hold it up
        // and wait for it to be pulled down.
        mode = 0;
    } else {
        // Already driven at rest: sense it float so the internal pull does not
        // fight the driver, and take whatever the resting level is as idle.
        mode = 2;
    }

    pinscan_sense(TOUCH_PIN, MODES[mode].pull);
    sleep_ms(20);
    idle         = gpio_read(TOUCH_PIN) ? 1 : 0;
    active_level = idle ? 0 : 1;

    draw_mono(Y_TITLE, "PC2 TOUCH", ST77XX_CYAN);
    fmt_mode(buf, MODES[mode].tag, pinscan_class_char(cls), idle);
    draw_mono(Y_MODE, buf, ST77XX_MAGENTA);
    draw_state(0);
    fmt_count(buf, 0, 0);
    draw_mono(Y_COUNT, buf, ST77XX_WHITE);
    fmt_last(buf, 0);
    draw_mono(Y_LAST, buf, ST77XX_WHITE);

    // Trace rails
    gfx_draw_fast_h_line(0, TRACE_HI + 1, ST7735_TFTWIDTH, 0x2104);
    gfx_draw_fast_h_line(0, TRACE_LO - 1, ST7735_TFTWIDTH, 0x2104);

#if ENABLE_UART
    us("\n== PC2 touch key ==\nclass=");
    {
        char c[2];
        c[0] = pinscan_class_char(cls);
        c[1] = '\0';
        us(c);
    }
    us(" mode=");
    us(MODES[mode].tag);
    us(" idle=");
    un(idle);
    us("\n");
    uflush();
#endif

    unsigned int presses = 0, longs = 0, last_ms = 0;
    unsigned int press_t0 = 0;
    uint8_t      state = idle;      // debounced level
    uint8_t      cand  = idle;      // level being confirmed
    int          streak = 0;
    int16_t      x = 0;
    int16_t      prev_y = idle ? TRACE_HI : TRACE_LO;

    while (1) {
        uint8_t raw = gpio_read(TOUCH_PIN) ? 1 : 0;

        // --- raw trace: sweep one pixel per sample, clearing ahead of itself
        int16_t y = raw ? TRACE_HI : TRACE_LO;
        gfx_fill_rect(x, Y_TRACE, 1, H_TRACE, ST77XX_BLACK);
        if (y != prev_y) {
            // Rail-to-rail riser, so even a single-sample blip is visible.
            gfx_draw_fast_v_line(x, TRACE_HI, TRACE_LO - TRACE_HI + 1, ST77XX_YELLOW);
        } else {
            gfx_draw_pixel(x, y, ST77XX_CYAN);
            gfx_draw_pixel(x, y + 1, ST77XX_CYAN);
        }
        prev_y = y;
        if (++x >= ST7735_TFTWIDTH) {
            x = 0;
            gfx_draw_fast_h_line(0, TRACE_HI + 1, ST7735_TFTWIDTH, 0x2104);
            gfx_draw_fast_h_line(0, TRACE_LO - 1, ST7735_TFTWIDTH, 0x2104);
        }

        // --- debounce
        if (raw == cand) {
            if (streak < DEBOUNCE) streak++;
        } else {
            cand   = raw;
            streak = 1;
        }

        if (streak >= DEBOUNCE && cand != state) {
            state = cand;

            if (state == active_level) {
                press_t0 = clock_time();
                draw_state(1);
            } else {
                unsigned int dur = (clock_time() - press_t0) / TICKS_PER_MS;
                presses++;
                if (dur >= LONG_MS) longs++;
                last_ms = dur;

                draw_state(0);
                fmt_count(buf, presses, longs);
                draw_mono(Y_COUNT, buf, ST77XX_YELLOW);
                fmt_last(buf, last_ms);
                draw_mono(Y_LAST, buf, (dur >= LONG_MS) ? ST77XX_GREEN : ST77XX_WHITE);

#if ENABLE_UART
                us("press #");
                un(presses);
                us(" dur=");
                un(last_ms);
                us("ms");
                us(dur >= LONG_MS ? " LONG\n" : "\n");
                uflush();
#endif
            }
        }

        sleep_ms(SAMPLE_MS);
    }

    return 0;
}
