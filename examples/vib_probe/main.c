/*
 * Vibrator motor hunt by battery sag — FitPro LT715 / TLSR8232
 *
 * vib_hunt and vib_sweep both depend on a human feeling a buzz. This one does
 * not: a motor spinning pulls tens of mA, which sags the battery rail, and the
 * board already has a VBAT/4 divider on PB1 that can see it. So drive each
 * unknown pin and watch what the load does to the rail.
 *
 * That makes it strictly more sensitive than the earlier sweeps:
 *   - it catches a motor too weak, too brief, or too well damped to feel
 *   - it catches a motor whose leads are off, since then nothing sags anywhere
 *   - it also catches contention: a pin fighting another IC's output draws
 *     current too, which is worth knowing before driving that pin any longer
 *
 * Columns: PIN, largest DC sag in mV, largest AC sag in mV, both held at their
 * maximum across passes so a one-off event is not lost. Anything at or above
 * SAG_HINT_MV is worth chasing; the rail's own noise is a few mV.
 *
 * IMPORTANT: run this on the LiPo, not off a bench supply or a USB LDO. A stiff
 * supply regulates the sag away and every column stays at zero.
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
#include "../../lib/pinscan/pinscan.h"

// UART logging on PB4/PB5. Turn off to probe PB4/PB5 as well.
#define ENABLE_UART   1

#define BATT_PIN      GPIO_PB1
#define BATT_SCALE    4      // 1:4 divider on the PCB

#define AC_HZ         205    // mid-band for an LRA; an ERM spins on this too
#define DC_MS         250    // drive time before the rail is sampled
#define AC_BURST_MS   40     // AC toggling between samples
#define AC_BURSTS     6
#define SETTLE_MS     150    // rail recovery between steps
#define SAG_HINT_MV   15

#define MAX_PINS      8

#define Y_TITLE       2
#define Y_BASE        14
#define Y_HEAD        28
#define ROW_Y0        42
#define ROW_H         12

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

static volatile signed short g_adc_buf[128];

// Same sampling path as examples/battery_pct: misc channel into a RAM buffer,
// four settled samples averaged, scaled back up through the divider.
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
        if (g_adc_buf[i] & (1 << 13))
            samples[i] = 0;
        else
            samples[i] = g_adc_buf[i] & 0x1FFF;
    }

    adc_power_on(0);
    adc_aif_set_m_chn_en(0);

    unsigned int avg = (samples[2] + samples[3] + samples[4] + samples[5]) / 4;
    unsigned int pin_mv = (avg * 295) >> 8;
    return pin_mv * BATT_SCALE;
}

// Lowest of n reads — a sag is a dip, so the minimum is the signal.
static unsigned int batt_min_mv(int n) {
    unsigned int lo = 0xFFFFFFFF;
    int          i;
    for (i = 0; i < n; i++) {
        unsigned int v = batt_read_mv();
        if (v && v < lo) lo = v;
    }
    return (lo == 0xFFFFFFFF) ? 0 : lo;
}

static void drive_open(GPIO_PinTypeDef pin, uint8_t start) {
    gpio_set_func(pin, AS_GPIO);
    gpio_write(pin, start);
    gpio_set_input_en(pin, 0);
    gpio_setup_up_down_resistor(pin, GPIO_PULL_UP_DOWN_FLOAT);
    gpio_set_output_en(pin, 1);
}

static void ac_burst(GPIO_PinTypeDef pin, unsigned int hz, unsigned int ms) {
    unsigned int half_us = 500000 / hz;
    unsigned int cycles  = (ms * 500) / half_us;
    unsigned int i;
    for (i = 0; i < cycles; i++) {
        gpio_write(pin, 1);
        sleep_us(half_us);
        gpio_write(pin, 0);
        sleep_us(half_us);
    }
}

static const pinscan_cand_t *scan[MAX_PINS];
static unsigned int          sag_dc[MAX_PINS];
static unsigned int          sag_ac[MAX_PINS];
static int                   nscan;

static void draw_mono(int16_t y, const char *s, uint16_t color) {
    gfx_set_text_color_bg(color, ST77XX_BLACK);
    gfx_set_cursor(2, y);
    gfx_print(s);
}

static void fmt3(char *out, unsigned int v) {
    if (v > 999) v = 999;
    out[0] = (char)('0' + (v / 100) % 10);
    out[1] = (char)('0' + (v / 10) % 10);
    out[2] = (char)('0' + v % 10);
}

// "PA3 012 034"
static void fmt_row(char *out, int i) {
    out[0] = scan[i]->name[0];
    out[1] = scan[i]->name[1];
    out[2] = scan[i]->name[2];
    out[3] = ' ';
    fmt3(out + 4, sag_dc[i]);
    out[7] = ' ';
    fmt3(out + 8, sag_ac[i]);
    out[11] = '\0';
}

// "vbat 3987mV"
static void fmt_base(char *out, unsigned int mv) {
    if (mv > 9999) mv = 9999;
    out[0] = 'v'; out[1] = 'b'; out[2] = 'a'; out[3] = 't'; out[4] = ' ';
    out[5] = (char)('0' + (mv / 1000) % 10);
    out[6] = (char)('0' + (mv / 100) % 10);
    out[7] = (char)('0' + (mv / 10) % 10);
    out[8] = (char)('0' + mv % 10);
    out[9] = 'm'; out[10] = 'V';
    out[11] = '\0';
}

static uint16_t sag_color(unsigned int mv) {
    if (mv >= SAG_HINT_MV * 3) return ST77XX_GREEN;
    if (mv >= SAG_HINT_MV)     return ST77XX_YELLOW;
    return ST77XX_WHITE;
}

static void draw_rows(void) {
    char row[12];
    int  i;
    for (i = 0; i < nscan; i++) {
        unsigned int worst = (sag_dc[i] > sag_ac[i]) ? sag_dc[i] : sag_ac[i];
        fmt_row(row, i);
        draw_mono(ROW_Y0 + i * ROW_H, row, sag_color(worst));
    }
}

int main() {
    char buf[12];
    int  i;

    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

#if ENABLE_UART
    uart_helper_init(rx_buf, sizeof(rx_buf));
#endif

    for (i = 0; i < PINSCAN_CAND_COUNT; i++)
        pinscan_idle(PINSCAN_CAND[i].pin);

    adc_init();
    adc_base_init(BATT_PIN);

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_set_font(&FreeMono9pt7b);
    gfx_fill_screen(ST77XX_BLACK);

    nscan = 0;
    for (i = 0; i < PINSCAN_CAND_COUNT && nscan < MAX_PINS; i++) {
#if ENABLE_UART
        if (PINSCAN_CAND[i].uart_pad) continue;
#endif
        scan[nscan]   = &PINSCAN_CAND[i];
        sag_dc[nscan] = 0;
        sag_ac[nscan] = 0;
        nscan++;
    }

    draw_mono(Y_TITLE, "VIB PROBE", ST77XX_CYAN);
    draw_mono(Y_HEAD, "PIN  DC  AC", ST77XX_MAGENTA);
    draw_rows();

    while (1) {
        for (i = 0; i < nscan; i++) {
            GPIO_PinTypeDef pin = scan[i]->pin;
            unsigned int    base, load, sag;
            int             b;

            // Baseline with every candidate high-Z, taken right before the
            // drive so a slowly draining battery cannot masquerade as a sag.
            pinscan_idle(pin);
            sleep_ms(SETTLE_MS);
            base = batt_min_mv(3);
            fmt_base(buf, base);
            draw_mono(Y_BASE, buf, ST77XX_WHITE);

            // DC
            pinscan_drive(pin, 1);
            sleep_ms(DC_MS);
            load = batt_min_mv(3);
            pinscan_idle(pin);
            sag = (base > load) ? (base - load) : 0;
            if (sag > sag_dc[i]) sag_dc[i] = sag;

            sleep_ms(SETTLE_MS);

            // AC — toggle in bursts and sample between them. The rotor's own
            // inertia keeps the load up across the gaps.
            base = batt_min_mv(3);
            drive_open(pin, 0);
            load = 0xFFFFFFFF;
            for (b = 0; b < AC_BURSTS; b++) {
                unsigned int v;
                ac_burst(pin, AC_HZ, AC_BURST_MS);
                v = batt_read_mv();
                if (v && v < load) load = v;
            }
            pinscan_idle(pin);
            if (load == 0xFFFFFFFF) load = base;
            sag = (base > load) ? (base - load) : 0;
            if (sag > sag_ac[i]) sag_ac[i] = sag;

            draw_rows();

#if ENABLE_UART
            us(scan[i]->name);
            us(" dc=");
            un(sag_dc[i]);
            us("mV ac=");
            un(sag_ac[i]);
            us("mV base=");
            un(base);
            us("mV\n");
            uflush();
#endif
        }
    }

    return 0;
}
