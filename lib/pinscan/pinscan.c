#include "pinscan.h"

#include "drivers/5316/clock.h"

// TLSR8232 ET24 pins that are bonded out and not yet accounted for.
//
// Already known and therefore absent from this list:
//   PC7 (1) SWS, PA1 (2) LCD CS, PA6 (10) LCD RST, PB1 (11) VBAT/4 sense,
//   PB3 (13) backlight, PC1 (20) LCD DC, PC3 (22) LCD MOSI, PC5 (24) LCD CLK.
const pinscan_cand_t PINSCAN_CAND[] = {
    { GPIO_PA3, "PA3",  3, 0 },
    { GPIO_PA4, "PA4",  4, 0 },
    { GPIO_PA5, "PA5",  9, 0 },
    { GPIO_PB2, "PB2", 12, 0 },
    { GPIO_PC2, "PC2", 21, 0 },
    { GPIO_PC4, "PC4", 23, 0 },
    { GPIO_PB4, "PB4", 14, 1 },  // UART TX pad
    { GPIO_PB5, "PB5", 15, 1 },  // UART RX pad
};

const uint8_t PINSCAN_CAND_COUNT = sizeof(PINSCAN_CAND) / sizeof(PINSCAN_CAND[0]);

char pinscan_class_char(pinscan_class_t c) {
    switch (c) {
        case PINSCAN_FLOAT: return 'F';
        case PINSCAN_HIGH:  return 'H';
        case PINSCAN_LOW:   return 'L';
        default:            return '?';
    }
}

void pinscan_idle(GPIO_PinTypeDef pin) {
    gpio_set_func(pin, AS_GPIO);
    gpio_set_output_en(pin, 0);
    gpio_set_input_en(pin, 0);
    gpio_setup_up_down_resistor(pin, GPIO_PULL_UP_DOWN_FLOAT);
}

void pinscan_sense(GPIO_PinTypeDef pin, GPIO_PullTypeDef pull) {
    gpio_set_func(pin, AS_GPIO);
    gpio_set_output_en(pin, 0);
    gpio_set_input_en(pin, 1);
    gpio_setup_up_down_resistor(pin, pull);
}

void pinscan_drive(GPIO_PinTypeDef pin, uint8_t value) {
    gpio_set_func(pin, AS_GPIO);
    gpio_write(pin, value ? 1 : 0);
    gpio_set_input_en(pin, 0);
    gpio_setup_up_down_resistor(pin, GPIO_PULL_UP_DOWN_FLOAT);
    gpio_set_output_en(pin, 1);
}

pinscan_class_t pinscan_classify(GPIO_PinTypeDef pin) {
    unsigned char pu, pd;

    // 10K pull-up beats anything weaker than a real driver or a <10K divider.
    pinscan_sense(pin, GPIO_PULL_UP_10K);
    sleep_ms(2);
    pu = gpio_read(pin) ? 1 : 0;

    // 100K pull-down: only a floating pin follows it.
    pinscan_sense(pin, GPIO_PULL_DOWN_100K);
    sleep_ms(2);
    pd = gpio_read(pin) ? 1 : 0;

    pinscan_idle(pin);

    if (pu && !pd) return PINSCAN_FLOAT;
    if (pu && pd)  return PINSCAN_HIGH;
    if (!pu && !pd) return PINSCAN_LOW;
    return PINSCAN_ODD;
}
