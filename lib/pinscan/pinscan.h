#ifndef PINSCAN_H
#define PINSCAN_H

#include <stdint.h>

#include "drivers/5316/gpio.h"

// How an unknown pin behaves when probed as an input, first with the internal
// 10K pull-up and then with the internal 100K pull-down.
typedef enum {
    PINSCAN_FLOAT = 0,  // follows the pull  -> high-Z: unconnected, or a gate/base input
    PINSCAN_HIGH  = 1,  // high against pd   -> externally driven high, or hard pull-up
    PINSCAN_LOW   = 2,  // low against pu    -> externally driven low, or hard pull-down
    PINSCAN_ODD   = 3,  // inverted response -> noise / bad contact, re-probe
} pinscan_class_t;

typedef struct {
    GPIO_PinTypeDef pin;
    char            name[4];   // "PA3"
    uint8_t         mcu_pin;   // ET24 package pin number
    uint8_t         uart_pad;  // 1 = doubles as the FPC UART debug pad (PB4/PB5)
} pinscan_cand_t;

// Pins bonded out on the TLSR8232F512/F128 ET24 package whose board function is
// still unknown. PB4/PB5 are in the list but are flagged, because the UART
// helper claims them — skip them when UART logging is enabled.
extern const pinscan_cand_t PINSCAN_CAND[];
extern const uint8_t        PINSCAN_CAND_COUNT;

// Single-letter class code for compact on-screen tables: F / H / L / ?
char pinscan_class_char(pinscan_class_t c);

// Probe a pin as an input under both internal pulls and classify the result.
// Leaves the pin idle (see pinscan_idle).
pinscan_class_t pinscan_classify(GPIO_PinTypeDef pin);

// Electrically invisible: no output driver, no input buffer, no pull.
void pinscan_idle(GPIO_PinTypeDef pin);

// Input with the input buffer on and the given internal pull.
void pinscan_sense(GPIO_PinTypeDef pin, GPIO_PullTypeDef pull);

// Plain GPIO output. The level is latched before the driver is enabled, so the
// pin never glitches through the opposite level on the way out.
void pinscan_drive(GPIO_PinTypeDef pin, uint8_t value);

#endif // PINSCAN_H
