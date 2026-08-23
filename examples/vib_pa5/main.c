/*
 * PA5 drive sweep with load detection — FitPro LT715 / TLSR8232
 *
 * Decompiling the stock firmware puts the vibrator on PA5 (PA_OUT bit 5, the
 * literal at 0x0000ceb4 holds 0x800583). examples/vibrate drives it as DC pulses
 * and the motor does not move, so the question is no longer *which pin* but
 * *what waveform*.
 *
 * The stock code toggles the pin once per pass of a periodic task dispatcher and
 * stops after 4, 8 or 0x18 toggles. If that tick is fast, the motor is being fed
 * a square wave rather than DC — which is what an LRA needs and what a plain ERM
 * does not care about. So sweep DC plus a range of frequencies.
 *
 * Every step is scored by how far the battery rail sags, read through the VBAT/4
 * divider on PB1, so a motor that draws current registers even if it is too weak
 * or too damped to feel. Run on the LiPo: a bench supply regulates the sag away
 * and every row reads zero.
 */

#include "drivers/5316/adc.h"
#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/timer.h"

#include "../../lib/display/display.h"
#include "../../lib/display/gfx.h"
#include "../../lib/fonts/FreeMono9pt7b.h"
#include "../../lib/vibrate/vibrate.h"

#define BATT_PIN     GPIO_PB1
#define BATT_SCALE   4
#define DRIVE_MS     600
#define SETTLE_MS    250
#define SAG_HINT_MV  15

#define ROW_Y0       40
#define ROW_H        12

_attribute_ram_code_ void irq_handler(void) {}

// 0 means DC (held high for the whole step).
static const unsigned int HZ[] = { 0, 10, 25, 50, 100, 150, 200, 250, 300 };
#define NHZ (int)(sizeof(HZ) / sizeof(HZ[0]))

static unsigned int sag_mv[NHZ];

static volatile signed short g_adc_buf[128];

static unsigned int batt_read_mv(void) {
    int            i;
    unsigned short samples[8] = {0};

    adc_reset();
    aif_reset();
    adc_power_on(1);
    for (i = 0; i < 128; i++) g_adc_buf[i] = 0;
    sleep_us(25);
    adc_aif_set_misc_buf((unsigned short *)g_adc_buf, 128);
    adc_aif_set_m_chn_en(1);
    adc_aif_set_use_raw_data_en();

    unsigned int t0 = clock_time();
    for (i = 0; i < 8; i++) {
        while (!g_adc_buf[i] && !clock_time_exceed(t0, 20))
            ;
        t0 = clock_time();
        samples[i] = (g_adc_buf[i] & (1 << 13)) ? 0 : (g_adc_buf[i] & 0x1FFF);
    }
    adc_power_on(0);
    adc_aif_set_m_chn_en(0);

    unsigned int avg = (samples[2] + samples[3] + samples[4] + samples[5]) / 4;
    return ((avg * 295) >> 8) * BATT_SCALE;
}

static unsigned int batt_min(int n) {
    unsigned int lo = 0xFFFFFFFF;
    int i;
    for (i = 0; i < n; i++) {
        unsigned int v = batt_read_mv();
        if (v && v < lo) lo = v;
    }
    return lo == 0xFFFFFFFF ? 0 : lo;
}

// "DC    012" / "200Hz 034"
static void fmt_row(char *out, int i) {
    unsigned int hz = HZ[i], mv = sag_mv[i];
    if (mv > 999) mv = 999;
    if (hz == 0) {
        out[0]='D'; out[1]='C'; out[2]=' '; out[3]=' '; out[4]=' ';
    } else {
        out[0] = hz >= 100 ? (char)('0' + (hz/100)%10) : ' ';
        out[1] = hz >= 10  ? (char)('0' + (hz/10)%10)  : ' ';
        out[2] = (char)('0' + hz%10);
        out[3] = 'H'; out[4] = 'z';
    }
    out[5] = ' ';
    out[6] = (char)('0' + (mv/100)%10);
    out[7] = (char)('0' + (mv/10)%10);
    out[8] = (char)('0' + mv%10);
    out[9] = '\0';
}

static void draw(int16_t y, const char *s, uint16_t color) {
    gfx_set_text_color_bg(color, ST77XX_BLACK);
    gfx_set_cursor(2, y);
    gfx_print(s);
}

static void square(unsigned int hz, unsigned int ms) {
    unsigned int half_us = 500000 / hz;
    unsigned int cycles  = (ms * 500) / half_us;
    unsigned int i;
    for (i = 0; i < cycles; i++) {
        vibrate_set(1);
        sleep_us(half_us);
        vibrate_set(0);
        sleep_us(half_us);
    }
}

int main() {
    char buf[12];
    int  i;

    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    vibrate_init();
    adc_init();
    adc_base_init(BATT_PIN);

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_set_font(&FreeMono9pt7b);
    gfx_fill_screen(ST77XX_BLACK);

    // Positive control: blink the backlight, which is a known-good GPIO on the
    // same port style. If this does not blink, the fault is in our drive path,
    // not in PA5 or the motor.
    for (i = 0; i < 3; i++) {
        backlight_off();
        sleep_ms(150);
        backlight_on();
        sleep_ms(150);
    }

    draw(2,  "PA5 SWEEP", ST77XX_CYAN);
    draw(16, "sag in mV", ST77XX_MAGENTA);
    for (i = 0; i < NHZ; i++) {
        sag_mv[i] = 0;
        fmt_row(buf, i);
        draw(ROW_Y0 + i*ROW_H, buf, ST77XX_WHITE);
    }

    while (1) {
        for (i = 0; i < NHZ; i++) {
            unsigned int base, load, sag;

            vibrate_set(0);
            sleep_ms(SETTLE_MS);
            base = batt_min(3);

            if (HZ[i] == 0) {
                vibrate_set(1);
                sleep_ms(DRIVE_MS / 2);
                load = batt_min(3);
                vibrate_set(0);
            } else {
                // Toggle in bursts, sampling between them; the rotor's inertia
                // keeps the load up across the gaps.
                int b;
                load = 0xFFFFFFFF;
                for (b = 0; b < 5; b++) {
                    unsigned int v;
                    square(HZ[i], 60);
                    v = batt_read_mv();
                    if (v && v < load) load = v;
                }
                vibrate_set(0);
                if (load == 0xFFFFFFFF) load = base;
            }

            sag = base > load ? base - load : 0;
            if (sag > sag_mv[i]) sag_mv[i] = sag;

            draw(28, HZ[i] == 0 ? "now: DC   " : "now: sweep", ST77XX_CYAN);
            fmt_row(buf, i);
            draw(ROW_Y0 + i*ROW_H, buf,
                 sag_mv[i] >= SAG_HINT_MV*3 ? ST77XX_GREEN :
                 sag_mv[i] >= SAG_HINT_MV   ? ST77XX_YELLOW : ST77XX_WHITE);
        }
    }
    return 0;
}
