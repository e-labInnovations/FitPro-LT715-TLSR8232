#ifndef VIBRATE_H
#define VIBRATE_H

#include <stdint.h>

#include "drivers/5316/gpio.h"

// Vibrator motor on the FitPro LT715/LT716 — PA5 (package pin 9), active HIGH.
//
// Identified by decompiling the stock firmware (Ghidra 12 ships a Telink_TC32
// processor module). FUN_0000cdc4 there toggles PA_OUT bit 5 on an even/odd
// counter and stops once the counter passes a limit of 4, 8 or 0x18 depending on
// the alert type, and the BLE command handler at FUN_0000216c clears the same
// bit when the app cancels. Three further functions only ever clear it.
#define VIBRATE_PIN          GPIO_PA5
#define VIBRATE_ACTIVE_LEVEL 1

// The motor only turns on battery power. On an external 3.3 V supply the pin
// drives correctly and the rotor does not move — the inrush is more current than
// a bench supply or the 3V3 pad will give. Test on the LiPo.

// Stock toggles at a 300 ms half period (constant 0x493e0, compared << 4 against
// the system tick at 0x800740). Shorter works; that is the reference.

// Pulse counts the stock firmware uses. It counts toggles, so a "pulse" is two.
#define VIBRATE_SHORT_PULSES 2    // stock limit 4 toggles
#define VIBRATE_MEDIUM_PULSES 4   // stock limit 8
#define VIBRATE_LONG_PULSES  12   // stock limit 0x18

// Park the pin as an output, motor off. Call before the first pulse.
void vibrate_init(void);

// Motor on/off directly, for testing or for a custom pattern.
void vibrate_set(uint8_t on);

// Blocking: `pulses` on/off cycles of `ms` each.
void vibrate_pulses(unsigned int pulses, unsigned int ms);

#endif // VIBRATE_H
