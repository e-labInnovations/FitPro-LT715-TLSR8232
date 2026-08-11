#include "touch.h"

#include "drivers/5316/clock.h"

#define TICKS_PER_MS (sys_tick_per_us * 1000)

static uint8_t      s_state;        // debounced level
static uint8_t      s_cand;         // level waiting to be confirmed
static unsigned int s_cand_since;   // when s_cand was first seen
static unsigned int s_down_at;      // start of the current press
static unsigned int s_press_ms;     // duration of the current/last press
static unsigned int s_last_tap;     // timestamp of the previous tap release
static uint8_t      s_has_last_tap;
static uint8_t      s_long_fired;

static unsigned int ms_since(unsigned int ref) {
    return (unsigned int)(clock_time() - ref) / TICKS_PER_MS;
}

void touch_init(void) {
    // The key drives the pin both ways, so no internal pull — a pull would only
    // fight the driver and waste current.
    gpio_set_func(TOUCH_PIN_GPIO, AS_GPIO);
    gpio_set_output_en(TOUCH_PIN_GPIO, 0);
    gpio_set_input_en(TOUCH_PIN_GPIO, 1);
    gpio_setup_up_down_resistor(TOUCH_PIN_GPIO, GPIO_PULL_UP_DOWN_FLOAT);

    s_state        = 0;
    s_cand         = 0;
    s_cand_since   = clock_time();
    s_down_at      = 0;
    s_press_ms     = 0;
    s_last_tap     = 0;
    s_has_last_tap = 0;
    s_long_fired   = 0;
}

uint8_t touch_is_down(void) {
    return s_state;
}

unsigned int touch_press_ms(void) {
    return s_state ? ms_since(s_down_at) : s_press_ms;
}

const char *touch_event_name(touch_event_t e) {
    switch (e) {
        case TOUCH_DOWN:       return "DOWN";
        case TOUCH_UP:         return "UP";
        case TOUCH_TAP:        return "TAP";
        case TOUCH_DOUBLE_TAP: return "DTAP";
        case TOUCH_LONG_PRESS: return "LONG";
        default:               return "-";
    }
}

touch_event_t touch_poll(void) {
    uint8_t raw = (gpio_read(TOUCH_PIN_GPIO) ? 1 : 0) == TOUCH_ACTIVE_LEVEL;

    if (raw != s_cand) {
        s_cand       = raw;
        s_cand_since = clock_time();
    } else if (s_cand != s_state && ms_since(s_cand_since) >= TOUCH_DEBOUNCE_MS) {
        s_state = s_cand;

        if (s_state) {
            s_down_at    = clock_time();
            s_press_ms   = 0;
            s_long_fired = 0;
            return TOUCH_DOWN;
        }

        s_press_ms = ms_since(s_down_at);

        if (s_long_fired) {
            // The long press was already reported — do not also call it a tap.
            s_has_last_tap = 0;
            return TOUCH_UP;
        }

        if (s_has_last_tap && ms_since(s_last_tap) <= TOUCH_DTAP_MS) {
            s_has_last_tap = 0;
            return TOUCH_DOUBLE_TAP;
        }

        s_last_tap     = clock_time();
        s_has_last_tap = 1;
        return TOUCH_TAP;
    }

    // Long press fires while the key is still held, so the UI can react before
    // the finger lifts.
    if (s_state && !s_long_fired && ms_since(s_down_at) >= TOUCH_LONG_MS) {
        s_long_fired   = 1;
        s_has_last_tap = 0;
        return TOUCH_LONG_PRESS;
    }

    return TOUCH_NONE;
}
