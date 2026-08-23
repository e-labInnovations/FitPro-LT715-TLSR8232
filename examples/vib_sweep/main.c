/*
 * Vibrator motor hunt, second pass — FitPro LT715 / TLSR8232
 *
 * examples/vib_hunt drove each unknown pin high, low, and blinked it at ~4 Hz,
 * and nothing buzzed. That leaves four ways the motor could still hide, and this
 * sweep closes all of them:
 *
 *  1. AC drive. An LRA (linear resonant actuator) does not move on DC at all —
 *     it needs to be driven near its resonance, typically 150-250 Hz. A plain
 *     ERM coin motor spins on DC but also on any of these frequencies, so the
 *     sweep covers both kinds. This is the most likely miss.
 *  2. Census-skipped pins. vib_hunt refused to drive a pin the census reported
 *     as externally driven; a motor gate with a bleed resistor to GND looks
 *     exactly like that. Here every candidate is driven, class logged not obeyed.
 *  3. PB4/PB5. Set ENABLE_UART 0 to bring the two UART pads into the sweep.
 *  4. An enable pin. Some drivers need a second GPIO held high before the drive
 *     pin does anything. PAIR_PASS holds each candidate high in turn while
 *     pulsing every other one.
 *
 * Contention is the cost of ignoring the census: if a pin turns out to be
 * another IC's output, driving it pushes current through both drivers. Every
 * drive here is short and the pin is returned to high-Z between steps, but do
 * not leave this running unattended for hours.
 *
 * Tap the touch key (PC2) during the pause between pins to skip ahead.
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
#include "../../lib/touch/touch.h"

// UART logging on PB4/PB5. Turn off to sweep PB4/PB5 as well.
#define ENABLE_UART   1

#define PAIR_PASS     1     // after the single-pin pass, try enable+drive pairs
#define DC_MS         700   // solid HIGH / LOW drive time
#define AC_MS         400   // per frequency
#define GAP_MS        400   // pin high-Z between steps
#define AFTER_MS      1500  // pause between pins (tap to skip)

// Frequency sweep, Hz. 175-235 is where the LRAs in this class of watch sit.
static const unsigned int FREQS[] = { 50, 100, 150, 175, 205, 235, 300 };
#define NFREQ (int)(sizeof(FREQS) / sizeof(FREQS[0]))

#define MAX_PINS      8

#define Y_TITLE       2
#define Y_PIN         18
#define H_PIN         26
#define Y_PHASE       50
#define Y_PROG        64
#define Y_CLASS       78
#define Y_HINT        112

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

static void draw_mono(int16_t y, const char *s, uint16_t color) {
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color_bg(color, ST77XX_BLACK);
    gfx_set_cursor(2, y);
    gfx_print(s);
}

static void draw_pin(const char *s, uint16_t color) {
    gfx_fill_rect(0, Y_PIN, ST7735_TFTWIDTH, H_PIN, ST77XX_BLACK);
    gfx_set_font(&FreeSans12pt7b);
    gfx_set_text_color(color);
    gfx_set_cursor(4, Y_PIN + 3);
    gfx_print(s);
}

// Hold the pin as a push-pull output; the caller then toggles it with gpio_write.
static void drive_open(GPIO_PinTypeDef pin, uint8_t start) {
    gpio_set_func(pin, AS_GPIO);
    gpio_write(pin, start);
    gpio_set_input_en(pin, 0);
    gpio_setup_up_down_resistor(pin, GPIO_PULL_UP_DOWN_FLOAT);
    gpio_set_output_en(pin, 1);
}

// Square wave by software toggling — no PWM peripheral needed, and every
// candidate pin can do it regardless of which PWM channel it maps to.
static void ac_drive(GPIO_PinTypeDef pin, unsigned int hz, unsigned int ms) {
    unsigned int half_us = 500000 / hz;
    unsigned int cycles  = (ms * 500) / half_us;   // ms*1000 / (2*half_us)
    unsigned int i;

    drive_open(pin, 0);
    for (i = 0; i < cycles; i++) {
        gpio_write(pin, 1);
        sleep_us(half_us);
        gpio_write(pin, 0);
        sleep_us(half_us);
    }
    pinscan_idle(pin);
}

// "AC 175Hz" / "DC HIGH " — fixed width so redraws overwrite cleanly.
static void fmt_phase_ac(char *out, unsigned int hz) {
    out[0] = 'A'; out[1] = 'C'; out[2] = ' ';
    out[3] = (hz >= 100) ? (char)('0' + (hz / 100) % 10) : ' ';
    out[4] = (hz >= 10)  ? (char)('0' + (hz / 10) % 10)  : ' ';
    out[5] = (char)('0' + hz % 10);
    out[6] = 'H'; out[7] = 'z'; out[8] = ' ';
    out[9] = '\0';
}

// "03/06 c=F" — two digits, so the pair pass (up to 56 steps) still fits.
static void fmt_prog(char *out, int idx, int total, char cls) {
    unsigned int n = (unsigned int)(idx + 1);
    unsigned int t = (unsigned int)total;
    if (n > 99) n = 99;
    if (t > 99) t = 99;
    out[0] = (char)('0' + (n / 10) % 10);
    out[1] = (char)('0' + n % 10);
    out[2] = '/';
    out[3] = (char)('0' + (t / 10) % 10);
    out[4] = (char)('0' + t % 10);
    out[5] = ' ';
    out[6] = 'c'; out[7] = '=';
    out[8] = cls;
    out[9] = '\0';
}

// Pause between pins; a tap on the touch key cuts it short.
static void gap_or_tap(unsigned int ms) {
    unsigned int t0 = clock_time();
    while (!clock_time_exceed(t0, ms * 1000)) {
        touch_event_t ev = touch_poll();
        if (ev == TOUCH_TAP || ev == TOUCH_DOUBLE_TAP) return;
        sleep_ms(5);
    }
}

// One pin: DC high, the frequency sweep, then DC low.
static void sweep_pin(int idx) {
    GPIO_PinTypeDef pin = scan[idx]->pin;
    char            buf[12];
    int             f;

    draw_pin(scan[idx]->name, ST77XX_WHITE);
    fmt_prog(buf, idx, nscan, pinscan_class_char((pinscan_class_t)scan_class[idx]));
    draw_mono(Y_PROG, buf, ST77XX_MAGENTA);

#if ENABLE_UART
    us("--- ");
    us(scan[idx]->name);
    us(" pin ");
    un(scan[idx]->mcu_pin);
    us("\n");
    uflush();
#endif

    draw_mono(Y_PHASE, "DC HIGH  ", ST77XX_RED);
    pinscan_drive(pin, 1);
    sleep_ms(DC_MS);
    pinscan_idle(pin);
    sleep_ms(GAP_MS);

    for (f = 0; f < NFREQ; f++) {
        fmt_phase_ac(buf, FREQS[f]);
        draw_mono(Y_PHASE, buf, ST77XX_YELLOW);
        ac_drive(pin, FREQS[f], AC_MS);
        sleep_ms(GAP_MS);
    }

    draw_mono(Y_PHASE, "DC LOW   ", ST77XX_BLUE);
    pinscan_drive(pin, 0);
    sleep_ms(DC_MS);
    pinscan_idle(pin);

    draw_mono(Y_PHASE, "idle     ", ST77XX_GREEN);
    gap_or_tap(AFTER_MS);
}

#if PAIR_PASS
// hold_idx held high as a would-be enable, drive_idx pulsed DC then 205 Hz.
static void sweep_pair(int hold_idx, int drive_idx, int n, int total) {
    char buf[12];
    char label[8];

    label[0] = scan[hold_idx]->name[0];
    label[1] = scan[hold_idx]->name[1];
    label[2] = scan[hold_idx]->name[2];
    label[3] = '+';
    label[4] = scan[drive_idx]->name[0];
    label[5] = scan[drive_idx]->name[1];
    label[6] = scan[drive_idx]->name[2];
    label[7] = '\0';

    draw_pin(label, ST77XX_CYAN);
    fmt_prog(buf, n, total, 'P');
    draw_mono(Y_PROG, buf, ST77XX_MAGENTA);

#if ENABLE_UART
    us("--- hold ");
    us(scan[hold_idx]->name);
    us(" drive ");
    us(scan[drive_idx]->name);
    us("\n");
    uflush();
#endif

    pinscan_drive(scan[hold_idx]->pin, 1);

    draw_mono(Y_PHASE, "DC HIGH  ", ST77XX_RED);
    pinscan_drive(scan[drive_idx]->pin, 1);
    sleep_ms(DC_MS);
    pinscan_idle(scan[drive_idx]->pin);
    sleep_ms(GAP_MS);

    draw_mono(Y_PHASE, "AC 205Hz ", ST77XX_YELLOW);
    ac_drive(scan[drive_idx]->pin, 205, AC_MS);

    pinscan_idle(scan[hold_idx]->pin);

    draw_mono(Y_PHASE, "idle     ", ST77XX_GREEN);
    gap_or_tap(GAP_MS * 2);
}
#endif

int main() {
    int i, j;

    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

#if ENABLE_UART
    uart_helper_init(rx_buf, sizeof(rx_buf));
#endif

    for (i = 0; i < PINSCAN_CAND_COUNT; i++)
        pinscan_idle(PINSCAN_CAND[i].pin);

    touch_init();

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_fill_screen(ST77XX_BLACK);

    // Census is recorded for the log only — every candidate gets driven here.
    nscan = 0;
    for (i = 0; i < PINSCAN_CAND_COUNT && nscan < MAX_PINS; i++) {
#if ENABLE_UART
        if (PINSCAN_CAND[i].uart_pad) continue;
#endif
        scan[nscan]       = &PINSCAN_CAND[i];
        scan_class[nscan] = (uint8_t)pinscan_classify(PINSCAN_CAND[i].pin);
        nscan++;
    }

    draw_mono(Y_TITLE, "VIB SWEEP", ST77XX_CYAN);
    draw_mono(Y_HINT, "buzz? note", ST77XX_WHITE);
    draw_pin("HOLD IT", ST77XX_YELLOW);
    gap_or_tap(2500);

    while (1) {
        for (i = 0; i < nscan; i++)
            sweep_pin(i);

#if PAIR_PASS
        {
            int n = 0;
            int total = nscan * (nscan - 1);
            draw_mono(Y_CLASS, "pair pass", ST77XX_CYAN);
            for (i = 0; i < nscan; i++)
                for (j = 0; j < nscan; j++) {
                    if (i == j) continue;
                    sweep_pair(i, j, n++, total);
                }
            draw_mono(Y_CLASS, "         ", ST77XX_BLACK);
        }
#endif
    }

    return 0;
}
