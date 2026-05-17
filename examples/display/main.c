/*
Author: Mohammed Ashad (e-labInnovations)
Date: 2026-05-17
Description: Display test program for FitPro LT715/715 smart watch with the TLS8232 chip
*/

#include <stdint.h>
#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/timer.h"
#include "drivers/5316/uart.h"

// 16 Mhz.
#define SYSTEM_CLOCK 16000000

// SPI clock = SYSTEM_CLOCK / (2 * (SPI_DIV + 1))
#define SPI_DIV 3

// LP715 Watch Pin Mapping
#define PIN_SDA GPIO_PC3   // MOSI          - FPC 3
#define PIN_SCL GPIO_PC5   // CLK           - FPC 4
#define PIN_DC  GPIO_PC1   // DC            - FPC 5
#define PIN_RST GPIO_PA6   // RST           - FPC 6
#define PIN_CS  GPIO_PA1   // CS            - FPC 7
#define PIN_BL  GPIO_PB3   // Backlight     - FPC 11

// Required by linker
_attribute_ram_code_ void irq_handler(void) {}

// ST7735 Commands
#define TFT_CMD_NRON     0x13
#define TFT_CMD_ON       0x29
#define TFT_CMD_MADCTL   0x36
#define TFT_CMD_CASET    0x2a
#define TFT_CMD_RASET    0x2b
#define TFT_CMD_COLMOD   0x3a
#define TFT_CMD_RAMWR    0x2c
#define TFT_CMD_SLEEPOUT 0x11
#define TFT_CMD_SWRESET  0x01
#define TFT_CMD_INVON    0x21

// Display dimensions - adjust if needed
#define TFT_WIDTH  128
#define TFT_HEIGHT 128
#define TFT_X_OFFSET 2
#define TFT_Y_OFFSET 1

// Colors (RGB565)
#define TFT_COLOR_BLACK 0x0000
#define TFT_COLOR_WHITE 0xffff
#define TFT_COLOR_RED   0xf800
#define TFT_COLOR_GREEN 0x07e0
#define TFT_COLOR_BLUE  0x001f

typedef struct {
    uint8_t cmd;
    uint8_t sleep_ms;
    uint8_t args_len;
    uint8_t args[];
} tft_cmd_t;

static const tft_cmd_t tft_cmd_swreset  = {.cmd = TFT_CMD_SWRESET,  .sleep_ms = 120, .args_len = 0, .args = {}};
static const tft_cmd_t tft_cmd_sleepout = {.cmd = TFT_CMD_SLEEPOUT, .sleep_ms = 120, .args_len = 0, .args = {}};
static const tft_cmd_t tft_cmd_madctl   = {.cmd = TFT_CMD_MADCTL,   .sleep_ms = 10,  .args_len = 1, .args = {0x00}};
static const tft_cmd_t tft_cmd_disp_on  = {.cmd = TFT_CMD_ON,       .sleep_ms = 120, .args_len = 0, .args = {}};
static const tft_cmd_t tft_cmd_normal   = {.cmd = TFT_CMD_NRON,     .sleep_ms = 10,  .args_len = 0, .args = {}};
static const tft_cmd_t tft_cmd_colmod   = {.cmd = TFT_CMD_COLMOD,   .sleep_ms = 10,  .args_len = 1, .args = {0x05}};
static const tft_cmd_t tft_cmd_invon    = {.cmd = TFT_CMD_INVON,    .sleep_ms = 10,  .args_len = 0, .args = {}};
static const tft_cmd_t tft_cmd_caset    = {
    .cmd = TFT_CMD_CASET, .sleep_ms = 10, .args_len = 4,
    .args = {0x00, 0x00, 0x00, TFT_WIDTH - 1}
};
static const tft_cmd_t tft_cmd_raset    = {
    .cmd = TFT_CMD_RASET, .sleep_ms = 10, .args_len = 4,
    .args = {0x00, 0x00, 0x00, TFT_HEIGHT - 1}
};

static void inline spi_write_data(const uint8_t *data, size_t len) {
    reg_spi_ctrl &= ~(FLD_SPI_DATA_OUT_DIS | FLD_SPI_RD);
    while (len--) {
        reg_spi_data = *data++;
        while (reg_spi_ctrl & FLD_SPI_BUSY);
    }
}

static inline void spi_write8(uint8_t data)   { return spi_write_data(&data, 1); }
static inline void spi_write16(uint16_t data) { spi_write8(data >> 8); spi_write8(data & 0xff); }

static void spi_send_cmd(uint8_t cmd) {
    gpio_write(PIN_DC, 0);
    spi_write_data(&cmd, 1);
    gpio_write(PIN_DC, 1);
}

static void tft_send_cmd(const tft_cmd_t *cmd) {
    gpio_write(PIN_CS, 0);
    gpio_write(PIN_DC, 0);
    spi_write_data(&cmd->cmd, 1);
    gpio_write(PIN_DC, 1);
    spi_write_data(cmd->args, cmd->args_len);
    gpio_write(PIN_CS, 1);
    sleep_ms(cmd->sleep_ms);
}

static void tft_set_window(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    spi_send_cmd(TFT_CMD_CASET);
    x += TFT_X_OFFSET;
    spi_write16(x);
    spi_write16(x + w - 1);

    spi_send_cmd(TFT_CMD_RASET);
    y += TFT_Y_OFFSET;
    spi_write16(y);
    spi_write16(y + h - 1);

    spi_send_cmd(TFT_CMD_RAMWR);
}

static void tft_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    gpio_write(PIN_CS, 0);
    tft_set_window(x, y, w, h);
    for (int i = 0; i < w * h; i++) {
        spi_write16(color);
    }
    gpio_write(PIN_CS, 1);
}

static void tft_clear_screen() {
    return tft_draw_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_COLOR_BLACK);
}

static void spi_init_master(void) {
    spi_master_init(SPI_DIV, SPI_MODE0);

    gpio_set_func(PIN_SDA, AS_SPI_MDO);
    gpio_set_func(PIN_SCL, AS_SPI_MCK);
    gpio_set_output_en(PIN_SDA, 1);
    gpio_set_output_en(PIN_SCL, 1);
    gpio_set_data_strength(PIN_SDA, 0);
    gpio_set_data_strength(PIN_SCL, 0);

    gpio_set_func(PIN_CS, AS_GPIO);
    gpio_set_input_en(PIN_CS, 0);
    gpio_set_output_en(PIN_CS, 1);
    gpio_write(PIN_CS, 1);

    gpio_set_func(PIN_DC, AS_GPIO);
    gpio_set_input_en(PIN_DC, 0);
    gpio_set_output_en(PIN_DC, 1);
    gpio_write(PIN_DC, 0);

    gpio_set_func(PIN_RST, AS_GPIO);
    gpio_set_input_en(PIN_RST, 0);
    gpio_set_output_en(PIN_RST, 1);
    gpio_write(PIN_RST, 1);
}

static void tft_init_display(void) {
    gpio_set_func(PIN_BL, AS_GPIO);
    gpio_set_input_en(PIN_BL, 0);
    gpio_set_output_en(PIN_BL, 1);
    gpio_write(PIN_BL, 1);

    gpio_write(PIN_RST, 0);
    sleep_ms(100);
    gpio_write(PIN_RST, 1);
    sleep_ms(100);

    tft_send_cmd(&tft_cmd_swreset);
    tft_send_cmd(&tft_cmd_sleepout);
    tft_send_cmd(&tft_cmd_madctl);
    tft_send_cmd(&tft_cmd_colmod);
    tft_send_cmd(&tft_cmd_invon);
    tft_send_cmd(&tft_cmd_disp_on);
    tft_send_cmd(&tft_cmd_caset);
    tft_send_cmd(&tft_cmd_raset);
    tft_clear_screen();
}

int main() {
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    spi_init_master();
    tft_init_display();

    // Draw colored rectangles to test display
    uint8_t side = TFT_WIDTH;
    tft_draw_rect(0,            0,            side,     side,     TFT_COLOR_RED);
    tft_draw_rect(10,           10,           side-20,  side-20,  TFT_COLOR_GREEN);
    tft_draw_rect(20,           20,           side-40,  side-40,  TFT_COLOR_BLUE);

    while (1);

    return 0;
}