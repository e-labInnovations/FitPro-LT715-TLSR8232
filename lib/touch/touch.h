#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>

#include "drivers/5316/gpio.h"

// Touch key on the FitPro LT715/LT716 — PC2 (package pin 21).
//
// Measured with examples/touch_key: the pin is an actively driven push-pull
// output (census class L — held low at rest), goes HIGH for as long as the key
// is touched, and needs no internal pull. So: sense float, active high, and the
// level tracks the finger rather than emitting a fixed-width pulse.

#define TOUCH_PIN_GPIO     GPIO_PC2
#define TOUCH_ACTIVE_LEVEL 1

// Timing. Poll at least every ~10 ms for these to hold. Override any of them by
// #define-ing it before including this header.
//
// The key is a slow one: the IC keeps the line high for a while after the finger
// leaves, so an ordinary tap on this watch measures 400-550 ms. TOUCH_LONG_MS
// therefore sits well above that, not at the ~500 ms a snappier button would use
// — otherwise every normal touch reads as a long press.
#ifndef TOUCH_DEBOUNCE_MS
#define TOUCH_DEBOUNCE_MS  24
#endif
#ifndef TOUCH_LONG_MS
#define TOUCH_LONG_MS      1400  // held this long -> TOUCH_LONG_PRESS, once
#endif
#ifndef TOUCH_DTAP_MS
#define TOUCH_DTAP_MS      500   // second tap within this of the first -> double
#endif

typedef enum {
    TOUCH_NONE = 0,
    TOUCH_DOWN,        // debounced press started
    TOUCH_UP,          // released after a long press (the tap was already consumed)
    TOUCH_TAP,         // released before TOUCH_LONG_MS
    TOUCH_DOUBLE_TAP,  // second tap within TOUCH_DTAP_MS of the previous one
    TOUCH_LONG_PRESS,  // fired while still held, exactly once per press
} touch_event_t;

// Configure the pin. Safe to call before the display is brought up.
void touch_init(void);

// Advance the state machine and return at most one event per call. Release
// yields TOUCH_TAP / TOUCH_DOUBLE_TAP, or TOUCH_UP when TOUCH_LONG_PRESS
// already fired for that press — so a long press never also reads as a tap.
touch_event_t touch_poll(void);

// Debounced live state.
uint8_t touch_is_down(void);

// Milliseconds held: the running total while down, the final total once up.
unsigned int touch_press_ms(void);

// Short name for an event, for logging and on-screen display.
const char *touch_event_name(touch_event_t e);

#endif // TOUCH_H
