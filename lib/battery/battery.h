#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>

#include "drivers/5316/gpio.h"

// Battery gauge and charger detection for the FitPro LT715/LT716, taken from
// the stock firmware rather than invented.
//
// Both channels come out of the decompiled image (see reversed/ and the README
// section "Battery and charging, from the firmware"):
//
//   PB1  battery, through the PCB's 1:4 divider.  FUN_0000e870 samples it and
//        scales avg * 57 / 100 + 71, i.e. 0.57 mV per count - a 4.8 V full
//        scale, which is what a 1.2 V reference behind a 1:4 divider gives.
//   PB2  the charger rail, no divider.  FUN_0000c37c selects channel 0x104
//        (port 1, bit 2 = PB2) and scales (avg * 426 - 990) / 385, i.e. 1.107 mV
//        per count - a ~9.6 V full scale, which is the /8 prescaler. USB 5 V
//        lands mid-scale.
//
// PB2 was on our unknown-pin list, written off as "bulk init only". It is the
// charger sense input.

#define BATTERY_PIN        GPIO_PB1
#define BATTERY_DIVIDER    4        // 1:4 on the PCB
#define CHARGER_PIN        GPIO_PB2

// The stock gauge clamps to this range before doing anything else.
#define BATTERY_FULL_MV    4200
#define BATTERY_EMPTY_MV   3350

// Below this the stock firmware raises its low-battery state (FUN_0000c2ac
// calls the low-battery setter and pushes a screen). Same value as the gauge
// floor, so 0% and "low" coincide.
#define BATTERY_LOW_MV     3350

// Charging window on the PB2 channel, in millivolts of that rail. The lower
// bound is the presence test; the upper bound rejects a reading too high to be
// a USB supply. From FUN_0000c37c: not charging when
// ((reading - 4400) & 0xffff) > 2100.
#define CHARGER_PRESENT_MIN_MV 4400
#define CHARGER_PRESENT_MAX_MV 6500

// Percent as the stock firmware reports it. Note it is a five-step gauge, not a
// continuous curve: 4100 mV and up reads 100, and everything from 3800 to 4099
// also reads 100, so the top of the range is deliberately flat.
//
//   >= 4100 mV -> 100      >= 3600 mV -> 50
//   >= 3800 mV -> 100      >= 3500 mV -> 25
//   >= 3700 mV ->  75      <  3500 mV ->  0
uint8_t battery_percent(unsigned int mv);

// True once the pack is at or below BATTERY_LOW_MV.
uint8_t battery_is_low(unsigned int mv);

// Configure the ADC. Call once, after clock_init.
void battery_init(void);

// Battery voltage in mV. Median-of-16 like the stock firmware: it samples 16
// times, sorts, throws away the four highest and four lowest, and averages the
// middle eight, which is what makes the reading survive the motor and the
// backlight switching on.
unsigned int battery_read_mv(void);

// Charger rail voltage in mV on PB2. Around 0 when unplugged, near 5000 on USB.
unsigned int charger_read_mv(void);

// True when charger_read_mv() falls inside the stock window.
uint8_t charger_is_present(void);

#endif // BATTERY_H
