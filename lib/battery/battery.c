#include "battery.h"

#include "drivers/5316/adc.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"

// The stock gauge, verbatim from FUN_0000a020. Two parallel tables: walk the
// thresholds until the reading is at or above one, and take the level at that
// index. Both tables live at 0x19e28 and 0x19e40 in the stock image.
#define GAUGE_STEPS 6

static const unsigned short GAUGE_MV[GAUGE_STEPS]  = {4100, 3800, 3700, 3600, 3500, 3350};
static const unsigned char  GAUGE_PCT[GAUGE_STEPS] = { 100,  100,   75,   50,   25,    0};

uint8_t battery_percent(unsigned int mv) {
    int i;

    // The stock code clamps first, so a reading above full still reports 100
    // and a collapsed rail reports 0 instead of wrapping.
    if (mv > BATTERY_FULL_MV)  mv = BATTERY_FULL_MV;
    if (mv < BATTERY_EMPTY_MV) mv = BATTERY_EMPTY_MV;

    for (i = 0; i < GAUGE_STEPS; i++) {
        if (mv >= GAUGE_MV[i]) return GAUGE_PCT[i];
    }
    return 0;
}

uint8_t battery_is_low(unsigned int mv) {
    return (uint8_t)(mv <= BATTERY_LOW_MV);
}

// The ADC delivers into this buffer by DMA, so it has to be in RAM and volatile.
static volatile signed short g_adc_buf[128];

void battery_init(void) {
    adc_init();
    adc_base_init(BATTERY_PIN);
}

// One channel's worth of raw counts, median-filtered the way the stock firmware
// does it: 16 samples, sorted, four discarded off each end, middle eight
// averaged. Worth the extra samples - the motor and the backlight both dent the
// rail, and a plain average tracks those dents.
static unsigned int read_counts(GPIO_PinTypeDef pin) {
    unsigned short s[16];
    int            i, j;
    unsigned int   t0, sum;

    adc_base_init(pin);
    adc_reset();
    aif_reset();
    adc_power_on(1);

    for (i = 0; i < 128; i++) g_adc_buf[i] = 0;
    sleep_us(25);

    adc_aif_set_misc_buf((unsigned short *)g_adc_buf, 128);
    adc_aif_set_m_chn_en(1);
    adc_aif_set_use_raw_data_en();

    t0 = clock_time();
    for (i = 0; i < 16; i++) {
        while (!g_adc_buf[i] && !clock_time_exceed(t0, 20))
            ;
        t0 = clock_time();
        // bit 13 marks an out-of-range conversion; the stock code treats those
        // as zero rather than clamping, and the median then ignores them
        s[i] = (g_adc_buf[i] & (1 << 13)) ? 0 : (g_adc_buf[i] & 0x1FFF);
    }

    adc_power_on(0);
    adc_aif_set_m_chn_en(0);

    // insertion sort, 16 elements
    for (i = 1; i < 16; i++) {
        unsigned short v = s[i];
        for (j = i - 1; j >= 0 && s[j] > v; j--) s[j + 1] = s[j];
        s[j + 1] = v;
    }

    sum = 0;
    for (i = 4; i < 12; i++) sum += s[i];
    return sum >> 3;
}

// Our ADC path is configured by adc_base_init: /8 prescaler against the 1.2 V
// reference, which is 1.152 mV per count at the pin (295/256). That is our own
// calibration, not the stock firmware's - the stock image uses a different
// prescaler for the battery channel and so carries a different constant. The
// volts at the end are the same volts.
static unsigned int counts_to_pin_mv(unsigned int counts) {
    return (counts * 295) >> 8;
}

unsigned int battery_read_mv(void) {
    return counts_to_pin_mv(read_counts(BATTERY_PIN)) * BATTERY_DIVIDER;
}

unsigned int charger_read_mv(void) {
    // No divider on PB2: the /8 prescaler alone puts full scale near 9.4 V, so
    // a 5 V charger reads directly.
    return counts_to_pin_mv(read_counts(CHARGER_PIN));
}

uint8_t charger_is_present(void) {
    unsigned int mv = charger_read_mv();
    return (uint8_t)(mv >= CHARGER_PRESENT_MIN_MV && mv <= CHARGER_PRESENT_MAX_MV);
}
