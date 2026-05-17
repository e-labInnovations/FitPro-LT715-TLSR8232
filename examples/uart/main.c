/*
 * UART example for FitPro LT715 / TLSR8232
 * Ported from m6-reveng/example-programs/uart
 *
 * Sends "Tick counter: #N" over UART at 115200 baud every 500ms.
 * TX => GPIO_PB4  (FPC debug pad)
 * RX => GPIO_PB5  (FPC debug pad)
 */

#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/timer.h"
#include "common/string.h"

// Declare my_sprintf from SDK common/printf.c without pulling in printf.h
// (printf.h redefines sprintf to nothing unless BLE sample flags are set)
extern int my_sprintf(char *s, const char *fmt, ...);
#include "../../lib/uart/uart_helper.h"

_attribute_ram_code_ void irq_handler(void) {}

// TX send buffer
uart_data_t send_buff;

// RX buffer (unused in this example)
uint8_t recv_buff[32];

int main() {
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();

    uart_helper_init(recv_buff, sizeof(recv_buff));

    int tick = 0;
    while (1) {
        my_sprintf((char *)send_buff.data, "Tick counter: #%d\n", tick++);
        send_buff.len = strlen((char *)send_buff.data);
        uart_send(&send_buff);
        sleep_ms(500);
    }
    return 0;
}
