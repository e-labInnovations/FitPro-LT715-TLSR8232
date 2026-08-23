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
//   PB2  the charger rail, through a 1:8 divider on the PCB.  FUN_0000c37c
//        selects channel 0x104 (port 1, bit 2 = PB2) and scales
//        (avg * 426 - 990) / 385 = 1.107 mV per count *of the rail*, so the
//        stock firmware reads that channel with no prescaler (~1.13 V full
//        scale at the pin) and lets the board's divider provide the range.
//
// PB2 was on our unknown-pin list, written off as "bulk init only". It is the
// charger sense input.
//
// The divider ratio was measured, not assumed. Unplugged, the node reads
// VBAT/8: 515 mV of pin against a 4092 mV pack is a ratio of 7.95. On the
// charger it reads 653 mV, i.e. 5224 mV of rail - a USB supply. The node
// follows whichever of VBAT and VBUS is higher, which is exactly why the stock
// window starts at 4400 mV: that is above a full battery (4200 mV), so a rail
// reading above it can only mean an external supply.

#define BATTERY_PIN        GPIO_PB1
#define BATTERY_DIVIDER    4        // 1:4 on the PCB
#define CHARGER_PIN        GPIO_PB2
#define CHARGER_DIVIDER    8        // 1:8 on the PCB, measured against VBAT

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

// Charger rail voltage in mV, i.e. the PB2 pin reading scaled back up through
// the 1:8 divider. Reads roughly the battery voltage when unplugged (the node
// follows VBAT) and about 5200 on USB - not zero, so do not test it against
// zero. Use charger_is_present() or the window constants.
unsigned int charger_read_mv(void);

// The raw pin voltage on PB2, before the divider. Only useful for calibration -
// if charger_read_mv() disagrees with a multimeter, this is the number to
// compare against.
unsigned int charger_read_pin_mv(void);

// True when charger_read_mv() falls inside the stock window.
uint8_t charger_is_present(void);

#endif // BATTERY_H
