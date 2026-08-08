#include "display.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/spi.h"
#include "drivers/5316/timer.h"

// SPI clock = 16MHz / (2 * (SPI_DIV + 1))
//
// 0 is 8 MHz, the fastest this peripheral reaches from a 16 MHz system clock,
// and it is a straight 4x on every pixel pushed. The panel is not the limit:
// the usb2tft project drives this same salvaged ST7735 from an RP2040 at
// 31.25 MHz. 3 (2 MHz) was a conservative bring-up value that was never revised
// once the panel proved good.
//
// Override from the build if a particular set of leads turns out to be noisy —
// 1 gives 4 MHz. Tearing or speckle on the flex is what that would look like.
#ifndef SPI_DIV
#define SPI_DIV 0
#endif

uint16_t _display_width  = ST7735_TFTWIDTH;
uint16_t _display_height = ST7735_TFTHEIGHT;

static uint8_t _colstart = 0;
static uint8_t _rowstart = 0;
static uint8_t _tabcolor = INITR_GREENTAB;
static uint8_t _rotation = 0;

// ---------------------------------------------------------------------------
// SPI low-level
// ---------------------------------------------------------------------------

static inline void spi_write_byte(uint8_t data) {
    reg_spi_data = data;
    while (reg_spi_ctrl & FLD_SPI_BUSY);
}

static void spi_write_buf(const uint8_t *buf, uint32_t len) {
    reg_spi_ctrl &= ~(FLD_SPI_DATA_OUT_DIS | FLD_SPI_RD);
    while (len--) {
        reg_spi_data = *buf++;
        while (reg_spi_ctrl & FLD_SPI_BUSY);
    }
}

// ---------------------------------------------------------------------------
// ST7735 command/data helpers
// ---------------------------------------------------------------------------

static inline void st_cmd(uint8_t cmd) {
    gpio_write(PIN_DC, 0);
    spi_write_byte(cmd);
    gpio_write(PIN_DC, 1);
}

static inline void st_data8(uint8_t data) {
    spi_write_byte(data);
}

static inline void st_data16(uint16_t data) {
    spi_write_byte(data >> 8);
    spi_write_byte(data & 0xFF);
}

// ---------------------------------------------------------------------------
// Init command list executor
// Format: [num_commands, cmd, num_args|0x80_for_delay, [delay_ms,] args...]
// ---------------------------------------------------------------------------

static void exec_cmd_list(const uint8_t *addr) {
    uint8_t numCommands = *addr++;
    while (numCommands--) {
        uint8_t cmd     = *addr++;
        uint8_t numArgs = *addr++;
        uint16_t ms     = (numArgs & 0x80) ? *addr++ : 0;
        numArgs &= 0x7F;

        gpio_write(PIN_CS, 0);
        st_cmd(cmd);
        while (numArgs--) st_data8(*addr++);
        gpio_write(PIN_CS, 1);

        if (ms) {
            if (ms == 255) ms = 500;
            sleep_ms(ms);
        }
    }
}

// ---------------------------------------------------------------------------
// ST7735 init sequences (same as amir1387aht / Adafruit)
// ---------------------------------------------------------------------------

static const uint8_t Rcmd1[] = {
    15,
    ST77XX_SWRESET,   0x80, 10,
    ST77XX_SLPOUT,    0x80, 10,
    ST7735_FRMCTR1, 3,  0x01, 0x2C, 0x2D,
    ST7735_FRMCTR2, 3,  0x01, 0x2C, 0x2D,
    ST7735_FRMCTR3, 6,  0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,
    ST7735_INVCTR,  1,  0x07,
    ST7735_PWCTR1,  3,  0xA2, 0x02, 0x84,
    ST7735_PWCTR2,  1,  0xC5,
    ST7735_PWCTR3,  2,  0x0A, 0x00,
    ST7735_PWCTR4,  2,  0x8A, 0x2A,
    ST7735_PWCTR5,  2,  0x8A, 0xEE,
    ST7735_VMCTR1,  1,  0x0E,
    ST77XX_INVOFF,  0,
    ST77XX_MADCTL,  1,  0xC8,
    ST77XX_COLMOD,  1,  0x05,
};

static const uint8_t Rcmd2green[] = {
    2,
    ST77XX_CASET, 4,  0x00, 0x02, 0x00, 0x7F + 0x02,
    ST77XX_RASET, 4,  0x00, 0x01, 0x00, 0x9F + 0x01,
};

static const uint8_t Rcmd2red[] = {
    2,
    ST77XX_CASET, 4,  0x00, 0x00, 0x00, 0x7F,
    ST77XX_RASET, 4,  0x00, 0x00, 0x00, 0x9F,
};

static const uint8_t Rcmd3[] = {
    4,
    ST7735_GMCTRP1, 16,
        0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d,
        0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16,
        0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
        0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,
    ST77XX_NORON,  0x80, 10,
    ST77XX_DISPON, 0x80, 10,
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void display_set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    st_cmd(ST77XX_CASET);
    st_data8(0x00); st_data8(x0 + _colstart);
    st_data8(0x00); st_data8(x1 + _colstart);

    st_cmd(ST77XX_RASET);
    st_data8(0x00); st_data8(y0 + _rowstart);
    st_data8(0x00); st_data8(y1 + _rowstart);

    st_cmd(ST77XX_RAMWR);
}

void display_set_rotation(uint8_t m) {
    m %= 4;
    _rotation = m;
    uint8_t madctl = 0;

    if (_tabcolor == INITR_GREENTAB) {
        _colstart = (m == 0 || m == 2) ? 2 : 1;
        _rowstart = (m == 0 || m == 1) ? 1 : 2;
    }

    switch (m) {
        case 0: madctl = ST7735_MADCTL_BGR; break;
        case 1: madctl = ST77XX_MADCTL_MV | ST7735_MADCTL_BGR; break;
        case 2: madctl = ST77XX_MADCTL_MY | ST77XX_MADCTL_MX | ST7735_MADCTL_BGR; break;
        case 3: madctl = ST77XX_MADCTL_MV | ST77XX_MADCTL_MY | ST7735_MADCTL_BGR; break;
    }

    gpio_write(PIN_CS, 0);
    st_cmd(ST77XX_MADCTL);
    st_data8(madctl);
    gpio_write(PIN_CS, 1);
}

void display_fill_screen(uint16_t color) {
    gpio_write(PIN_CS, 0);
    display_set_addr_window(0, 0, _display_width - 1, _display_height - 1);
    uint32_t total = (uint32_t)_display_width * _display_height;
    while (total--) st_data16(color);
    gpio_write(PIN_CS, 1);
}

void display_fill_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {
    uint32_t total = (uint32_t)(x1 - x0 + 1) * (y1 - y0 + 1);
    gpio_write(PIN_CS, 0);
    display_set_addr_window(x0, y0, x1, y1);
    while (total--) st_data16(color);
    gpio_write(PIN_CS, 1);
}

void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    display_fill_window(x, y, x + w - 1, y + h - 1, color);
}

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= _display_width || y >= _display_height) return;
    gpio_write(PIN_CS, 0);
    display_set_addr_window(x, y, x, y);
    st_data16(color);
    gpio_write(PIN_CS, 1);
}

void display_draw_image_ex(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                           const uint16_t *data, uint16_t src_stride) {
    if (x >= _display_width || y >= _display_height || !w || !h) return;

    // Clip to the right/bottom edges
    if (x + w > _display_width)  w = _display_width  - x;
    if (y + h > _display_height) h = _display_height - y;

    gpio_write(PIN_CS, 0);
    display_set_addr_window(x, y, x + w - 1, y + h - 1);
    for (uint8_t row = 0; row < h; row++) {
        const uint16_t *p = data + (uint32_t)row * src_stride;
        for (uint8_t col = 0; col < w; col++) st_data16(*p++);
    }
    gpio_write(PIN_CS, 1);
}

void display_draw_image(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *data) {
    display_draw_image_ex(x, y, w, h, data, w);
}

void display_draw_image_rle(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                            const uint16_t *rle, uint16_t runs) {
    if (x >= _display_width || y >= _display_height || !w || !h) return;

    uint8_t vw = (x + w > _display_width)  ? (uint8_t)(_display_width  - x) : w;
    uint8_t vh = (y + h > _display_height) ? (uint8_t)(_display_height - y) : h;

    gpio_write(PIN_CS, 0);
    display_set_addr_window(x, y, x + vw - 1, y + vh - 1);

    // Walk the runs in row-major order, dropping the pixels clipped off the
    // right edge so exactly vw*vh pixels reach the address window.
    uint8_t col = 0, row = 0;
    for (uint16_t r = 0; r < runs && row < vh; r++) {
        uint16_t count = rle[r * 2];
        uint16_t color = rle[r * 2 + 1];
        while (count--) {
            if (col < vw) st_data16(color);
            if (++col == w) {
                col = 0;
                if (++row == vh) break;
            }
        }
    }
    gpio_write(PIN_CS, 1);
}

uint16_t display_color(uint8_t r, uint8_t g, uint8_t b) {
    // Plain RGB565. The panel's colour filter is wired B-G-R, so MADCTL keeps
    // the BGR bit set (see display_set_rotation) and the swap happens in
    // hardware — the data we send is ordinary RGB565.
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void backlight_on(void)  { gpio_write(PIN_BL, 1); }
void backlight_off(void) { gpio_write(PIN_BL, 0); }

void display_init(uint8_t tabcolor, uint8_t rotation) {
    _tabcolor = tabcolor;

    // GPIO setup
    gpio_set_func(PIN_MOSI, AS_SPI_MDO);
    gpio_set_func(PIN_SCL,  AS_SPI_MCK);
    gpio_set_output_en(PIN_MOSI, 1);
    gpio_set_output_en(PIN_SCL,  1);

    gpio_set_func(PIN_CS,  AS_GPIO); gpio_set_input_en(PIN_CS,  0); gpio_set_output_en(PIN_CS,  1); gpio_write(PIN_CS,  1);
    gpio_set_func(PIN_DC,  AS_GPIO); gpio_set_input_en(PIN_DC,  0); gpio_set_output_en(PIN_DC,  1); gpio_write(PIN_DC,  1);
    gpio_set_func(PIN_RST, AS_GPIO); gpio_set_input_en(PIN_RST, 0); gpio_set_output_en(PIN_RST, 1); gpio_write(PIN_RST, 1);
    gpio_set_func(PIN_BL,  AS_GPIO); gpio_set_input_en(PIN_BL,  0); gpio_set_output_en(PIN_BL,  1); gpio_write(PIN_BL,  0);

    // SPI init
    spi_master_init(SPI_DIV, SPI_MODE0);

    // Hardware reset
    gpio_write(PIN_RST, 0); sleep_ms(10);
    gpio_write(PIN_RST, 1); sleep_ms(10);

    // Init sequences
    exec_cmd_list(Rcmd1);
    if (tabcolor == INITR_GREENTAB) {
        exec_cmd_list(Rcmd2green);
        _colstart = 2; _rowstart = 1;
    } else {
        exec_cmd_list(Rcmd2red);
        _colstart = 0; _rowstart = 0;
    }
    exec_cmd_list(Rcmd3);

    display_set_rotation(rotation);
    display_fill_screen(ST77XX_BLACK);
    backlight_on();
}