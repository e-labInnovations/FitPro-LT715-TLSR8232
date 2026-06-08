#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/timer.h"
#include "drivers/5316/adc.h"
#include "../../lib/display/display.h"
#include "../../lib/display/gfx.h"
#include "../../lib/fonts/FreeSans12pt7b.h"

_attribute_ram_code_ void irq_handler(void) {}

// PB1 has a 1:4 resistor divider from VBAT on the PCB.
// adc_base_init configures it as high-Z analog input with /8 prescaler + 1.2V Vref.
// The ADC formula returns V_PB1 in mV; multiply by 4 to recover VBAT.
#define BATT_PIN        GPIO_PB1
#define BATT_SCALE      4
#define VBAT_FULL_MV    4200
#define VBAT_EMPTY_MV   3000

static volatile signed short g_adc_buf[128];

static unsigned int batt_read_mv(void) {
    int i;
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

static char *utoa(unsigned int v, char *buf) {
    char tmp[12];
    int i = 0;
    if (v == 0) { *buf++ = '0'; *buf = '\0'; return buf; }
    while (v > 0) { tmp[i++] = '0' + (v % 10); v /= 10; }
    while (i > 0) *buf++ = tmp[--i];
    *buf = '\0';
    return buf;
}

static unsigned int mv_to_pct(unsigned int mv) {
    if (mv >= VBAT_FULL_MV)  return 100;
    if (mv <= VBAT_EMPTY_MV) return 0;
    return (mv - VBAT_EMPTY_MV) * 100 / (VBAT_FULL_MV - VBAT_EMPTY_MV);
}

static void draw_battery_icon(int16_t x, int16_t y, unsigned int pct) {
    gfx_draw_rect(x, y, 40, 20, ST77XX_WHITE);
    gfx_fill_rect(x + 40, y + 6, 4, 8, ST77XX_WHITE);
    uint16_t fill_w = (unsigned int)36 * pct / 100;
    uint16_t color = (pct > 20) ? ST77XX_GREEN : ST77XX_RED;
    if (fill_w > 0)
        gfx_fill_rect(x + 2, y + 2, fill_w, 16, color);
}

int main() {
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    adc_init();
    adc_base_init(BATT_PIN);

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_set_font(&FreeSans12pt7b);

    while (1) {
        unsigned int batt_mv = batt_read_mv();
        unsigned int pct     = mv_to_pct(batt_mv);

        gfx_fill_screen(ST77XX_BLACK);

        draw_battery_icon(42, 10, pct);

        char buf[16];
        char *p = buf;
        p = utoa(pct, p);
        *p++ = '%'; *p = '\0';

        gfx_set_text_color(ST77XX_WHITE);
        gfx_set_cursor(45, 68);
        gfx_print(buf);

        char vbuf[16];
        p = vbuf;
        p = utoa(batt_mv / 1000, p);
        *p++ = '.';
        unsigned int frac = batt_mv % 1000;
        if (frac < 100) *p++ = '0';
        if (frac < 10)  *p++ = '0';
        p = utoa(frac, p);
        *p++ = ' '; *p++ = 'V'; *p = '\0';

        gfx_set_text_color(ST77XX_CYAN);
        gfx_set_cursor(20, 100);
        gfx_print(vbuf);

        sleep_ms(1000);
    }

    return 0;
}
