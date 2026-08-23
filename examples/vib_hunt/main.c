/*
 * Output pin hunt (vibrator motor) — FitPro LT715 / TLSR8232
 *
 * Sweeps the pins whose function is still unknown — PA3, PA4, PA5, PB2, PC2,
 * PC4 (+ PB4/PB5 when UART is off) — driving one at a time while the screen
 * says which pin and which phase is live. Hold the watch and note the pin name
 * shown when it buzzes.
 *
 * Per pin, three phases with the pin left floating in between:
 *     HIGH    driven to 3V3       -> finds an active-high low-side driver (NPN/NMOS gate)
 *     BLINK   4x on/off at ~4Hz   -> unmistakable stutter, tells a buzz from a knock
 *     LOW     driven to GND       -> finds an active-low high-side driver (PNP/PMOS gate)
 *
 * Safety: pins that the census (pinscan_classify) finds externally driven are
 * skipped, because driving against another IC's output means contention through
 * both drivers. Set FORCE_ALL to 1 to override — short pulses only.
 *
 * Once a pin buzzes, set ONLY_INDEX to that row's number to loop it alone.
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

// UART logging on PB4/PB5. Turn off to include PB4/PB5 in the sweep instead.
#define ENABLE_UART   1

#define PULSE_MS      400   // solid HIGH / LOW drive time
#define BLINKS        4
#define BLINK_MS      120
#define GAP_MS        500   // pin floating between phases
#define AFTER_MS      1200  // pin floating between pins
#define START_MS      3000  // countdown before the first pin

#define FORCE_ALL     0     // 1 = also drive pins the census says are driven
#define ONLY_INDEX    (-1)  // >=0 = loop that scan row forever

#define MAX_PINS      8

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

static const pinscan_cand_t *scan[MAX_PINS];
static uint8_t               scan_class[MAX_PINS];
static int                   nscan;

// Screen bands (y, height) — each phase redraw only repaints its own band.
#define BAND_TITLE_Y   4
#define BAND_PIN_Y     26
#define BAND_PIN_H     26
#define BAND_INFO_Y    58
#define BAND_PHASE_Y   80
#define BAND_PHASE_H   26
#define BAND_HINT_Y    112

static void draw_mono(int16_t y, const char *s, uint16_t color) {
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color_bg(color, ST77XX_BLACK);
    gfx_set_cursor(2, y);
    gfx_print(s);
}

static void draw_band(int16_t y, int16_t h, int16_t x, const char *s, uint16_t color) {
    gfx_fill_rect(0, y, ST7735_TFTWIDTH, h, ST77XX_BLACK);
    gfx_set_font(&FreeSans12pt7b);
    gfx_set_text_color(color);
    gfx_set_cursor(x, y);
    gfx_print(s);
}

// "pin 12  3/6"
static void fmt_info(char *out, int idx) {
    unsigned int p = scan[idx]->mcu_pin;
    out[0] = 'p'; out[1] = 'i'; out[2] = 'n'; out[3] = ' ';
    out[4] = (p >= 10) ? (char)('0' + p / 10) : ' ';
    out[5] = (char)('0' + p % 10);
    out[6] = ' '; out[7] = ' ';
    out[8]  = (char)('0' + idx + 1);
    out[9]  = '/';
    out[10] = (char)('0' + nscan);
    out[11] = '\0';
}

static void run_pin(int idx) {
    GPIO_PinTypeDef pin = scan[idx]->pin;
    char            info[12];
    int             b;

    gfx_fill_screen(ST77XX_BLACK);
    draw_mono(BAND_TITLE_Y, "VIB HUNT", ST77XX_CYAN);
    draw_band(BAND_PIN_Y, BAND_PIN_H, 38, scan[idx]->name, ST77XX_WHITE);
    fmt_info(info, idx);
    draw_mono(BAND_INFO_Y, info, ST77XX_MAGENTA);
    draw_mono(BAND_HINT_Y, "buzz? note", ST77XX_WHITE);

#if ENABLE_UART
    us("--- driving ");
    us(scan[idx]->name);
    us(" (pin ");
    un(scan[idx]->mcu_pin);
    us(")\n");
    uflush();
#endif

    draw_band(BAND_PHASE_Y, BAND_PHASE_H, 30, "HIGH", ST77XX_RED);
    pinscan_drive(pin, 1);
    sleep_ms(PULSE_MS);
    pinscan_idle(pin);
    sleep_ms(GAP_MS);

    draw_band(BAND_PHASE_Y, BAND_PHASE_H, 24, "BLINK", ST77XX_YELLOW);
    for (b = 0; b < BLINKS; b++) {
        pinscan_drive(pin, 1);
        sleep_ms(BLINK_MS);
        pinscan_idle(pin);
        sleep_ms(BLINK_MS);
    }
    sleep_ms(GAP_MS);

    draw_band(BAND_PHASE_Y, BAND_PHASE_H, 38, "LOW", ST77XX_BLUE);
    pinscan_drive(pin, 0);
    sleep_ms(PULSE_MS);
    pinscan_idle(pin);

    draw_band(BAND_PHASE_Y, BAND_PHASE_H, 34, "idle", ST77XX_GREEN);
    sleep_ms(AFTER_MS);
}

int main() {
    int i;

    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

#if ENABLE_UART
    uart_helper_init(rx_buf, sizeof(rx_buf));
#endif

    // Park every candidate first — a motor pin left enabled would buzz forever.
    for (i = 0; i < PINSCAN_CAND_COUNT; i++)
        pinscan_idle(PINSCAN_CAND[i].pin);

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_fill_screen(ST77XX_BLACK);

    // Census, then keep only the pins that are safe to drive.
    nscan = 0;
    for (i = 0; i < PINSCAN_CAND_COUNT && nscan < MAX_PINS; i++) {
#if ENABLE_UART
        if (PINSCAN_CAND[i].uart_pad) continue;
#endif
        pinscan_class_t cls = pinscan_classify(PINSCAN_CAND[i].pin);
#if !FORCE_ALL
        if (cls != PINSCAN_FLOAT) {
#if ENABLE_UART
            us("skip ");
            us(PINSCAN_CAND[i].name);
            us(" - externally driven\n");
            uflush();
#endif
            continue;
        }
#endif
        scan[nscan]       = &PINSCAN_CAND[i];
        scan_class[nscan] = (uint8_t)cls;
        nscan++;
    }

    if (nscan == 0) {
        draw_mono(BAND_TITLE_Y, "NO SAFE", ST77XX_RED);
        draw_mono(BAND_TITLE_Y + 14, "PINS", ST77XX_RED);
        while (1) sleep_ms(1000);
    }

    draw_mono(BAND_TITLE_Y, "VIB HUNT", ST77XX_CYAN);
    draw_mono(BAND_TITLE_Y + 20, "HOLD WATCH", ST77XX_YELLOW);
    sleep_ms(START_MS);

    while (1) {
        if (ONLY_INDEX >= 0 && ONLY_INDEX < nscan) {
            run_pin(ONLY_INDEX);
            continue;
        }
        for (i = 0; i < nscan; i++)
            run_pin(i);
    }

    return 0;
}
