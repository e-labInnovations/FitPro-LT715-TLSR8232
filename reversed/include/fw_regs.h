/* TLSR8232 hardware registers, by name.

Hand-written, not generated: this is the map that makes fw_data.h readable. The
address space is not part of the flash image, which is why the decompiler could
only ever show these as stores through pointers.

OEN is active LOW: a zero bit enables that pin's output driver. The stock
firmware writes PA_OEN = 0xdf, which enables exactly one pin - PA5, the
vibrator motor.
*/

#ifndef FW_REGS_H
#define FW_REGS_H

#define REG8(off)  (*(volatile unsigned char *)(0x800000u + (off)))
#define REG32(off) (*(volatile unsigned int  *)(0x800000u + (off)))

#define PA_IN    REG8(0x580)
#define PA_IE    REG8(0x581)
#define PA_OEN   REG8(0x582)
#define PA_OUT   REG8(0x583)
#define PA_POL   REG8(0x584)
#define PA_DS    REG8(0x585)
#define PA_FUNC  REG8(0x586)
#define PA_IRQ   REG8(0x587)

#define PB_IN    REG8(0x588)
#define PB_IE    REG8(0x589)
#define PB_OEN   REG8(0x58a)
#define PB_OUT   REG8(0x58b)
#define PB_POL   REG8(0x58c)
#define PB_DS    REG8(0x58d)
#define PB_FUNC  REG8(0x58e)
#define PB_IRQ   REG8(0x58f)

#define PC_IN    REG8(0x590)
#define PC_IE    REG8(0x591)
#define PC_OEN   REG8(0x592)
#define PC_OUT   REG8(0x593)
#define PC_POL   REG8(0x594)
#define PC_DS    REG8(0x595)
#define PC_FUNC  REG8(0x596)
#define PC_IRQ   REG8(0x597)

#define SYS_TICK REG32(0x740)   /* free-running, 16 MHz - SDK reg_system_tick */
#define IRQ_EN   REG8(0x643)    /* global interrupt gate - SDK reg_irq_en */

#define ANA_ADDR REG8(0x0b8)    /* analog register interface */
#define ANA_DATA REG8(0x0b9)
#define ANA_CTRL REG8(0x0ba)

/* Pins established by reverse engineering - see the README pin inventory. */
#define PIN_VIBRATE_BIT   0x20  /* PA5, active high, battery power only */
#define PIN_TOUCH_BIT     0x04  /* PC2, active high */
#define PIN_BACKLIGHT_BIT 0x08  /* PB3 */
#define PIN_LCD_CS_BIT    0x02  /* PA1 */
#define PIN_LCD_RST_BIT   0x40  /* PA6 */
#define PIN_LCD_DC_BIT    0x02  /* PC1 */
#define PIN_ACCEL_IRQ_BIT 0x10  /* PA4 */

#endif /* FW_REGS_H */
