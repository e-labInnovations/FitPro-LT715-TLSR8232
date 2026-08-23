/*
 * Input pin hunt — FitPro LT715 / TLSR8232
 *
 * Finds the touch key (and any other input) among the pins whose function is
 * still unknown: PA3, PA4, PA5, PB2, PC2, PC4 (+ PB4/PB5 when UART is off).
 *
 * Stage 1 - census. Each candidate is read as an input under the internal 10K
 * pull-up and then the internal 100K pull-down:
 *     F  follows the pull   -> high-Z, nothing driving it
 *     H  stays high vs pd   -> driven high / hard pull-up
 *     L  stays low  vs pu   -> driven low  / hard pull-down
 * Anything not F is being driven by the board, so do NOT drive it as an output
 * (see the vib_hunt example).
 *
 * Stage 2 - live monitor. Cycles through three sense modes, 10 s each, counting
 * every level change per pin. Touch / tap / hold the watch face during each
 * mode and watch which row's counter moves:
 *     PULLUP  catches a switch or open-drain output that pulls to GND
 *     PULLDN  catches a switch or output that pushes to 3V3
 *     FLOAT   catches an actively driven push-pull output (a touch IC)
 *
 * On-screen columns: PIN  C(lass)  L(evel)  CNT(edges this mode)
 * A row turns yellow as soon as it sees an edge, so a single quick tap is not
 * lost between redraws.
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
#include "../../lib/pinscan/pinscan.h"

// UART logging on PB4/PB5. Turn off to include PB4/PB5 in the scan instead.
#define ENABLE_UART   1

#define MODE_MS       10000  // time spent in each sense mode
#define SAMPLE_MS     4      // input polling interval
#define REDRAW_MS     150

#define ROW_H         12
#define ROW_Y0        2
#define MAX_ROWS      8

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
    const char      *label;
} sense_mode_t;

static const sense_mode_t MODES[] = {
    { GPIO_PULL_UP_10K,        "PULLUP 10K" },
    { GPIO_PULL_DOWN_100K,     "PULLDN100K" },
    { GPIO_PULL_UP_DOWN_FLOAT, "FLOAT     " },
};
#define NMODES (int)(sizeof(MODES) / sizeof(MODES[0]))

// Per-candidate state, indexed the same way as the filtered scan list below.
static const pinscan_cand_t *scan[MAX_ROWS];
static uint8_t               scan_class[MAX_ROWS];
static uint8_t               scan_level[MAX_ROWS];
static unsigned int          scan_edges[MAX_ROWS];
static int                   nscan;

// "PA3 F 1 007" — fixed width so a redraw overwrites the previous row exactly.
static void fmt_row(char *out, int i) {
    unsigned int cnt = scan_edges[i];
    if (cnt > 999) cnt = 999;
    out[0]  = scan[i]->name[0];
    out[1]  = scan[i]->name[1];
    out[2]  = scan[i]->name[2];
    out[3]  = ' ';
    out[4]  = pinscan_class_char((pinscan_class_t)scan_class[i]);
    out[5]  = ' ';
    out[6]  = '0' + scan_level[i];
    out[7]  = ' ';
    out[8]  = '0' + (char)((cnt / 100) % 10);
    out[9]  = '0' + (char)((cnt / 10) % 10);
    out[10] = '0' + (char)(cnt % 10);
    out[11] = '\0';
}

static void draw_text(int row, const char *s, uint16_t color) {
    gfx_set_text_color_bg(color, ST77XX_BLACK);
    gfx_set_cursor(2, ROW_Y0 + row * ROW_H);
    gfx_print(s);
}

static void draw_rows(void) {
    char row[12];
    int  i;
    for (i = 0; i < nscan; i++) {
        uint16_t color = ST77XX_WHITE;
        if (scan_edges[i] > 0)               color = ST77XX_YELLOW;
        if (scan_class[i] != PINSCAN_FLOAT)  color = ST77XX_CYAN;
        if (scan_edges[i] > 0 && scan_class[i] != PINSCAN_FLOAT) color = ST77XX_GREEN;
        fmt_row(row, i);
        draw_text(3 + i, row, color);
    }
}

int main() {
    int i;

    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

#if ENABLE_UART
    uart_helper_init(rx_buf, sizeof(rx_buf));
#endif

    // Park every candidate before anything else, so a pin that happens to drive
    // the vibrator is never left enabled while the census runs.
    for (i = 0; i < PINSCAN_CAND_COUNT; i++)
        pinscan_idle(PINSCAN_CAND[i].pin);

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);
    gfx_set_font(&FreeMono9pt7b);
    gfx_fill_screen(ST77XX_BLACK);

    // Build the scan list.
    nscan = 0;
    for (i = 0; i < PINSCAN_CAND_COUNT && nscan < MAX_ROWS; i++) {
#if ENABLE_UART
        if (PINSCAN_CAND[i].uart_pad) continue;
#endif
        scan[nscan]       = &PINSCAN_CAND[i];
        scan_class[nscan] = PINSCAN_FLOAT;
        scan_level[nscan] = 0;
        scan_edges[nscan] = 0;
        nscan++;
    }

    // Stage 1 — census.
    for (i = 0; i < nscan; i++)
        scan_class[i] = (uint8_t)pinscan_classify(scan[i]->pin);

#if ENABLE_UART
    us("\n== pin census ==\n");
    uflush();
    for (i = 0; i < nscan; i++) {
        char c[2];
        c[0] = pinscan_class_char((pinscan_class_t)scan_class[i]);
        c[1] = '\0';
        us(scan[i]->name);
        us(" (pin ");
        un(scan[i]->mcu_pin);
        us(") ");
        us(c);
        us("\n");
        uflush();
    }
#endif

    draw_text(0, "INPUT HUNT", ST77XX_CYAN);
    draw_text(2, "PIN C L CNT", ST77XX_MAGENTA);

    int          mode      = 0;
    unsigned int mode_t0   = clock_time();
    unsigned int redraw_t0 = clock_time();
    int          fresh     = 1;

    while (1) {
        if (fresh) {
            // Enter a mode: apply the pull, settle, then latch the resting
            // level without counting it as an edge.
            for (i = 0; i < nscan; i++)
                pinscan_sense(scan[i]->pin, MODES[mode].pull);
            sleep_ms(5);
            for (i = 0; i < nscan; i++) {
                scan_level[i] = gpio_read(scan[i]->pin) ? 1 : 0;
                scan_edges[i] = 0;
            }

            gfx_fill_rect(0, ROW_Y0 + ROW_H, ST7735_TFTWIDTH, ROW_H, ST77XX_BLACK);
            draw_text(1, MODES[mode].label, ST77XX_YELLOW);
            draw_rows();

#if ENABLE_UART
            us("mode ");
            us(MODES[mode].label);
            us(" - touch the watch now\n");
            uflush();
#endif
            fresh = 0;
        }

        for (i = 0; i < nscan; i++) {
            uint8_t lvl = gpio_read(scan[i]->pin) ? 1 : 0;
            if (lvl != scan_level[i]) {
                scan_level[i] = lvl;
                scan_edges[i]++;
            }
        }

        if (clock_time_exceed(redraw_t0, REDRAW_MS * 1000)) {
            redraw_t0 = clock_time();
            draw_rows();
        }

        if (clock_time_exceed(mode_t0, MODE_MS * 1000)) {
            mode_t0 = clock_time();
#if ENABLE_UART
            for (i = 0; i < nscan; i++) {
                if (!scan_edges[i]) continue;
                us("  ");
                us(scan[i]->name);
                us(" edges=");
                un(scan_edges[i]);
                us("\n");
                uflush();
            }
#endif
            mode  = (mode + 1) % NMODES;
            fresh = 1;
        }

        sleep_ms(SAMPLE_MS);
    }

    return 0;
}
