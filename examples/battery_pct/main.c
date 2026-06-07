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

// Convert unsigned int to decimal string, returns pointer to end of string.
static char *utoa(unsigned int v, char *buf) {
    char tmp[12];
    int i = 0;
    if (v == 0) { *buf++ = '0'; *buf = '\0'; return buf; }
    while (v > 0) { tmp[i++] = '0' + (v % 10); v /= 10; }
    while (i > 0) *buf++ = tmp[--i];
    *buf = '\0';
    return buf;
}

// LiPo: 3000 mV = 0%, 4200 mV = 100%
static unsigned int mv_to_pct(unsigned int mv) {
    if (mv >= 4200) return 100;
    if (mv <= 3000) return 0;
    return (mv - 3000) * 100 / 1200;
}

// Draw a simple battery icon at (x, y). w=40, h=20.
static void draw_battery_icon(int16_t x, int16_t y, unsigned int pct) {
    // Outline
    gfx_draw_rect(x, y, 40, 20, ST77XX_WHITE);
    // Tip
    gfx_fill_rect(x + 40, y + 6, 4, 8, ST77XX_WHITE);

    // Fill bar: 36px usable width inside border (1px border each side)
    uint16_t fill_w = (unsigned int)36 * pct / 100;
    uint16_t color = (pct > 20) ? ST77XX_GREEN : ST77XX_RED;
    if (fill_w > 0)
        gfx_fill_rect(x + 2, y + 2, fill_w, 16, color);
}

int main() {
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    // ADC init for battery voltage measurement.
    // adc_vbat_init drives the pin to VCC then measures it through the 1/8
    // prescaler (range 0-9.6V with Vref=1.2V), so no external divider needed.
    adc_init();
    adc_vbat_init(GPIO_PB0);

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_set_font(&FreeSans12pt7b);

    while (1) {
        unsigned int mv  = adc_set_sample_and_get_result();
        unsigned int pct = mv_to_pct(mv);

        gfx_fill_screen(ST77XX_BLACK);

        // Battery icon centered at top
        draw_battery_icon(42, 10, pct);

        // Percentage line: "XX%"
        char buf[16];
        char *p = buf;
        p = utoa(pct, p);
        *p++ = '%'; *p = '\0';

        gfx_set_text_color(ST77XX_WHITE);
        gfx_set_cursor(45, 68);
        gfx_print(buf);

        // Voltage line: "X.XXX V"
        char vbuf[16];
        p = vbuf;
        p = utoa(mv / 1000, p);       // integer volts
        *p++ = '.';
        unsigned int frac = mv % 1000;
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
