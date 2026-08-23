#include "vibrate.h"

#include "drivers/5316/clock.h"

void vibrate_init(void) {
    gpio_set_func(VIBRATE_PIN, AS_GPIO);
    gpio_write(VIBRATE_PIN, !VIBRATE_ACTIVE_LEVEL);
    gpio_set_input_en(VIBRATE_PIN, 0);
    gpio_setup_up_down_resistor(VIBRATE_PIN, GPIO_PULL_UP_DOWN_FLOAT);
    gpio_set_output_en(VIBRATE_PIN, 1);
}

void vibrate_set(uint8_t on) {
    gpio_write(VIBRATE_PIN, on ? VIBRATE_ACTIVE_LEVEL : !VIBRATE_ACTIVE_LEVEL);
}

void vibrate_pulses(unsigned int pulses, unsigned int ms) {
    unsigned int i;
    for (i = 0; i < pulses; i++) {
        vibrate_set(1);
        sleep_ms(ms);
        vibrate_set(0);
        sleep_ms(ms);
    }
}
