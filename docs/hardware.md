# Hardware

Pins, peripherals and test pads of the FitPro LT715/LT716 board.


| Component      | Details                                            |
| -------------- | -------------------------------------------------- |
| Watch model    | FitPro LT715/LT716                                 |
| MCU            | Telink TLSR8232F512/F128 ET24 (24-pin, no RST pin) |
| SOC ID         | `0x5316`                                           |
| Display        | ST7735 128x128 TFT                                 |
| Display module | XHZY14413-02                                       |
| BLE            | Built-in (TLSR8232)                                |

### Pin Inventory — TLSR8232F512/F128 ET24

All 24 package pins. Only 16 GPIOs are bonded out; PA0, PA2, PA7, PB0, PB6, PB7,
PC0 and PC6 have no pad on this package.

| Pin | GPIO | Alt functions (datasheet)                   | Board function      |
| --- | ---- | ------------------------------------------- | ------------------- |
| 1   | PC7  | SWS / PWM3 / ANA_C7                         | SWS debug pad       |
| 2   | PA1  | PWM3_N / UART_CTS / ANA_A1                  | LCD CS              |
| 3   | PA3  | PWM4 / UART_TX / I2C_MCK / DI / ANA_A3      | **unknown**         |
| 4   | PA4  | PWM2 / UART_RX / I2C_MSD / CK / ANA_A4      | IRQ input (accel?)  |
| 5   | —    | VDD3                                        | supply              |
| 6   | —    | DVSS                                        | ground              |
| 7   | —    | DVDDDEC                                     | supply              |
| 8   | —    | VDDDEC_F                                    | supply              |
| 9   | PA5  | PWM5 / I2C_CK / I2C_MCK / ANA_A5            | **Vibrator motor**  |
| 10  | PA6  | PWM4_N / I2C_SD / RX / CYC2LNA / ANA_A6     | LCD RST             |
| 11  | PB1  | PWM0 / MDO / TX / CYC2PA / ANA_B1 / pga_in0 | VBAT/4 sense        |
| 12  | PB2  | PWM2 / MDI / UART_CTS / ANA_B2 / pga_in1    | **unknown** (HR?)   |
| 13  | PB3  | PWM0 / MCK / UART_RTS / I2C_MSD / ANA_B3    | LCD backlight       |
| 14  | PB4  | PWM1_N / UART_TX / ANA_B4 / pga_in2         | UART TX pad         |
| 15  | PB5  | PWM4 / UART_RX / ANA_B5 / pga_in3           | UART RX pad (HR?)   |
| 16  | —    | XC2                                         | crystal             |
| 17  | —    | XC1                                         | crystal             |
| 18  | —    | AVDD3                                       | supply              |
| 19  | —    | ANT                                         | antenna             |
| 20  | PC1  | PWM2_N / ANA_C1                             | LCD DC/RS           |
| 21  | PC2  | CN / PWM0_N / MCN / UART_CTS / ANA_C2       | Touch key           |
| 22  | PC3  | DO / PWM5_N / MDO / UART_RTS / ANA_C3       | LCD MOSI            |
| 23  | PC4  | DI / I2C_MSD / UART_TX / ANA_C4             | **unknown**         |
| 24  | PC5  | CK / I2C_MCK / MCK / UART_RX / ANA_C5       | LCD CLK             |

Three pins are still unaccounted for — **PA3, PB2, PC4** — and the heart-rate
sensor is still to place. The vibrator (PA5) and an interrupt input that is
probably the accelerometer (PA4) came out of decompiling the stock firmware, not
out of pin sweeps: see [Reading the pins out of the firmware](pin-hunting.md#reading-the-pins-out-of-the-firmware).

### Display Pin Mapping

| Signal | GPIO | MCU Pin | FPC Pin |
| ------ | ---- | ------- | ------- |
| MOSI   | PC3  | 22      | 3       |
| CLK    | PC5  | 24      | 4       |
| DC/RS  | PC1  | 20      | 5       |
| RST    | PA6  | 10      | 6       |
| CS     | PA1  | 2       | 7       |
| GND    | —    | GND     | 8 / 13  |
| VCC    | —    | 3.3V    | 10      |
| BL     | PB3  | 13      | 11 / 12 |

> **Note**: Backlight `PB3` HIGH = ON (direct drive, not via NPN transistor)

### Battery Sensing

> The stock firmware's own gauge, thresholds and charger detection are decoded in
> [Battery and charging](battery.md) — including **PB2 as the
> charger sense input**, which the pin sweeps never found.

| Signal     | GPIO | Notes                                      |
| ---------- | ---- | ------------------------------------------ |
| VBAT sense | PB1  | 1:4 resistor divider from raw battery rail |

> **Note**: The TLSR8232's internal VBAT ADC channel reads VDD (regulated 3.3V), not the battery.
> PB1 reads VBAT/4 via a hardware resistor divider on the PCB. Multiply ADC result by 4 to recover
> actual battery voltage (3.0–4.2V range). Verified by GPIO scan with known battery voltages.

### Touch Key

| Signal    | GPIO | MCU Pin | Notes                                              |
| --------- | ---- | ------- | -------------------------------------------------- |
| Touch key | PC2  | 21      | Active **HIGH**, idle low, level follows the finger |

> Characterised with the `touch_key` example: census class `L` (the pin is
> actively driven low at rest) and it goes high for exactly as long as the key is
> touched — so it is a push-pull touch-IC output, not a fixed-width pulse. Sense
> it **float**: no internal pull is needed, and a pull would only fight the
> driver. Driver: [lib/touch/](../lib/touch/).

### Vibrator Motor

| Signal  | GPIO | MCU Pin | Notes                        |
| ------- | ---- | ------- | ---------------------------- |
| Motor   | PA5  | 9       | Active **HIGH**, plain GPIO  |

> Found by decompiling the stock firmware. `FUN_0000cdc4` toggles `PA_OUT` bit 5
> on an even/odd counter and stops once the counter passes a limit of 4, 8 or
> 0x18 — three buzz patterns chosen by alert type. The BLE command handler at
> `FUN_0000216c` clears the same bit when the app cancels, and three further
> functions only ever clear it. The toggle interval is a constant `0x493e0`
> compared as `<< 4` against the system tick at `0x800740`, i.e. a **300 ms half
> period**. Driver: [lib/vibrate/](../lib/vibrate/), confirmed on hardware by
> [examples/vibrate/](../examples/vibrate/) and [examples/vib_pa5/](../examples/vib_pa5/).

> #### ⚠️ The motor only runs on battery power
>
> `examples/vibrate` appeared to do nothing for a while: the screen cycled
> through SHORT/MEDIUM/LONG but the motor never moved. The firmware was right
> the whole time — the board was on an external 3.3 V supply. A vibrator pulls
> tens of milliamps in an inrush the bench supply and the on-board LDO path will
> not deliver, so the motor sits still while every GPIO reads correct. Reconnect
> the LiPo and the same unmodified binary buzzes.
>
> This applies to any current-hungry load, so **test motors, backlight brightness
> and radio transmit on the battery**, never on 3V3 alone. It also explains why
> [examples/vib_probe/](../examples/vib_probe/) insists on the LiPo: on a stiff
> supply the battery-sag column stays at zero no matter which pin is right.

### UART Debug (FPC pads)

| Signal | GPIO | Notes         |
| ------ | ---- | ------------- |
| TX     | PB4  | FPC debug pad |
| RX     | PB5  | FPC debug pad |

### PCB Test Pads

| Pad | Function                                 |
| --- | ---------------------------------------- |
| SWS | SWire debug interface (flash / recovery) |
| DAT | Secondary GPIO — not SWire               |
| 3V3 | 3.3V supply (post-LDO, not VBAT)         |
| GND | Ground                                   |

### Other Peripherals (not yet mapped)

> Heart rate sensor and accelerometer — GPIO assignments unknown. PA4 is an
> interrupt input and is the strongest accelerometer candidate (see *Reading the
> pins out of the firmware*). Now mapped: the vibrator (PA5), the touch key
> (PC2), the charger sense input (PB2) and PB5, which the stock firmware drives
> from the charger state — see *Battery and charging, out of the firmware*.
