#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/timer.h"
#include "../../lib/display/display.h"
#include "../../lib/display/gfx.h"
#include "../../lib/fonts/FreeSans12pt7b.h"

_attribute_ram_code_ void irq_handler(void) {}

int main() {
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    display_init(INITR_GREENTAB, 0);
    gfx_init(ST7735_TFTWIDTH, ST7735_TFTHEIGHT, 0);

    // Fill background
    gfx_fill_screen(ST77XX_BLACK);

    // Draw some shapes
    gfx_fill_round_rect(10, 10, 108, 108, 10, ST77XX_RED);
    gfx_fill_circle(64, 64, 30, ST77XX_BLUE);
    gfx_draw_circle(64, 64, 40, ST77XX_YELLOW);
    
    // Print text
    gfx_set_font(&FreeSans12pt7b);
    gfx_set_text_color(ST77XX_WHITE);
    
    // Note: custom fonts use a baseline Y coordinate
    gfx_set_cursor(30, 70); 
    gfx_print("Hello!");

    while (1);
    return 0;
}