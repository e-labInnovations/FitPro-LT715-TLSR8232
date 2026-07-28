#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

// ST77XX / ST7735 Commands
#define ST77XX_SWRESET    0x01
#define ST77XX_SLPOUT     0x11
#define ST77XX_NORON      0x13
#define ST77XX_INVOFF     0x20
#define ST77XX_INVON      0x21
#define ST77XX_DISPON     0x29
#define ST77XX_CASET      0x2A
#define ST77XX_RASET      0x2B
#define ST77XX_RAMWR      0x2C
#define ST77XX_MADCTL     0x36
#define ST77XX_COLMOD     0x3A

#define ST7735_FRMCTR1    0xB1
#define ST7735_FRMCTR2    0xB2
#define ST7735_FRMCTR3    0xB3
#define ST7735_INVCTR     0xB4
#define ST7735_PWCTR1     0xC0
#define ST7735_PWCTR2     0xC1
#define ST7735_PWCTR3     0xC2
#define ST7735_PWCTR4     0xC3
#define ST7735_PWCTR5     0xC4
#define ST7735_VMCTR1     0xC5
#define ST7735_GMCTRP1    0xE0
#define ST7735_GMCTRN1    0xE1

// MADCTL bits
#define ST77XX_MADCTL_MX  0x40
#define ST77XX_MADCTL_MY  0x80
#define ST77XX_MADCTL_MV  0x20
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08

// Display size
#define ST7735_TFTWIDTH   128
#define ST7735_TFTHEIGHT  128

// Display tab color options
#define INITR_GREENTAB    0x0
#define INITR_REDTAB      0x1

// Colors (RGB565 — the panel's B/G/R filter order is handled by the MADCTL
// BGR bit, so pixel data is ordinary RGB565)
#define ST77XX_BLACK      0x0000
#define ST77XX_WHITE      0xFFFF
#define ST77XX_RED        0xF800
#define ST77XX_GREEN      0x07E0
#define ST77XX_BLUE       0x001F
#define ST77XX_YELLOW     0xFFE0
#define ST77XX_CYAN       0x07FF
#define ST77XX_MAGENTA    0xF81F

// LP715 Watch Pin Mapping
#define PIN_MOSI  GPIO_PC3
#define PIN_SCL   GPIO_PC5
#define PIN_DC    GPIO_PC1
#define PIN_RST   GPIO_PA6
#define PIN_CS    GPIO_PA1
#define PIN_BL    GPIO_PB3

// Public API
void display_init(uint8_t tabcolor, uint8_t rotation);
void display_set_rotation(uint8_t m);
void display_set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void display_fill_screen(uint16_t color);
void display_fill_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);
void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
uint16_t display_color(uint8_t r, uint8_t g, uint8_t b);

// Color image blit — streams pixels into one address window instead of
// re-addressing per pixel, so it is far faster than looping draw_pixel.
// Pixel data is RGB565 (see display_color); generate it with tools/img2c.py.
void display_draw_image(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *data);
// Same, but reads a w x h window out of a wider source buffer.
void display_draw_image_ex(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                           const uint16_t *data, uint16_t src_stride);
// Run-length encoded variant: `rle` is `runs` pairs of (pixel_count, color).
void display_draw_image_rle(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                            const uint16_t *rle, uint16_t runs);
void backlight_on(void);
void backlight_off(void);

// Expose width/height for gfx layer
extern uint16_t _display_width;
extern uint16_t _display_height;

#endif // DISPLAY_H