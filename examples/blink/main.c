/*
 * Blink example for FitPro LT715 / TLSR8232
 * Ported from m6-reveng/example-programs/blinky
 *
 * Toggles the Tx pin (GPIO_PB4) every 500ms.
 */

#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/timer.h"
#include "drivers/5316/uart.h"

// FitPro LT715 Tx pin
#define PIN_TX GPIO_PB4

_attribute_ram_code_ void irq_handler(void) {}

int main() {
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    // Tx pin as output
    gpio_set_func(PIN_TX, AS_GPIO);
    gpio_set_output_en(PIN_TX, 1);
    gpio_set_input_en(PIN_TX, 0);
    gpio_write(PIN_TX, 1);

    while (1) {
        gpio_toggle(PIN_TX);
        sleep_ms(500);
    }
    return 0;
}
