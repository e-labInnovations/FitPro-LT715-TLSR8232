/********************************************************************************************************
 * @file     gpio.h
 *
 * @brief    This is the header file for TLSR8232
 *
 * @author	 junwei.lu
 * @date     May 8, 2018
 *
 * @par      Copyright (c) 2018, Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *           The information contained herein is confidential property of Telink
 *           Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *           of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *           Co., Ltd. and the licensee or the terms described here-in. This heading
 *           MUST NOT be removed from this file.
 *
 *           Licensees are granted free, non-transferable use of the information in this
 *           file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/

#pragma once

#include "driver_config.h"
#include "register.h"
#include "gpio_default.h"

typedef enum{
		GPIO_GROUPA    = 0x000,
		GPIO_GROUPB    = 0x100,
		GPIO_GROUPC    = 0x200,
		GPIO_GROUPD    = 0x300,
		GPIO_GROUPE    = 0x400,

	    GPIO_PA0 = 0x000 | BIT(0),
		GPIO_PA1 = 0x000 | BIT(1),
		GPIO_PA2 = 0x000 | BIT(2),
		GPIO_PA3 = 0x000 | BIT(3),
		GPIO_PA4 = 0x000 | BIT(4),
		GPIO_PA5 = 0x000 | BIT(5),
		GPIO_PA6 = 0x000 | BIT(6),
		GPIO_PA7 = 0x000 | BIT(7),

		GPIO_PB0 = 0x100 | BIT(0),
		GPIO_PB1 = 0x100 | BIT(1),
		GPIO_PB2 = 0x100 | BIT(2),
		GPIO_PB3 = 0x100 | BIT(3),
		GPIO_PB4 = 0x100 | BIT(4),
		GPIO_PB5 = 0x100 | BIT(5),
		GPIO_PB6 = 0x100 | BIT(6),
		GPIO_PB7 = 0x100 | BIT(7),

		GPIO_PC0 = 0x200 | BIT(0),
		GPIO_PC1 = 0x200 | BIT(1),
		GPIO_PC2 = 0x200 | BIT(2),
		GPIO_PC3 = 0x200 | BIT(3),
		GPIO_PC4 = 0x200 | BIT(4),
		GPIO_PC5 = 0x200 | BIT(5),
		GPIO_PC6 = 0x200 | BIT(6),
		GPIO_PC7 = 0x200 | BIT(7),

		GPIO_PE0 = 0x400 | BIT(0),
		GPIO_PE1 = 0x400 | BIT(1),
		GPIO_PE2 = 0x400 | BIT(2),
		GPIO_PE3 = 0x400 | BIT(3),


		GPIO_PE4 = 0x400 | BIT(4),
		GPIO_PE5 = 0x400 | BIT(5),
		GPIO_PE6 = 0x400 | BIT(6),

		GPIO_PF0 = 0x500 | BIT(0),

		GPIO_ALL  = 0x500,
		GPIO_NONE = 0xfff,
}GPIO_PinTypeDef;


typedef enum {
	GPIO_PULL_UP_DOWN_FLOAT = 0,
	GPIO_PULL_UP_1M     	= 1,
	GPIO_PULL_UP_10K 		= 2,
	GPIO_PULL_DOWN_100K  	= 3,
}GPIO_PullTypeDef;

typedef enum{
	GPIO_Level_Low  = 0x00,
	GPIO_Level_High = 0x01,
}GPIO_LevelTypeDef;

typedef enum{
	GPIO_Pol_rising  = 0x00,
	GPIO_Pol_falling = 0x01,
}GPIO_PolTypeDef;

typedef enum{
	AS_GPIO   = 0,
	AS_AF     = (!0),
	AS_MSPI   = 1,
	AS_SWIRE  = 2,
	AS_UART   = 3,
	AS_PWM	   = 4,
	AS_I2C	   = 5,
	AS_SPI	   = 6,
	AS_ETH_MAC= 7,
	AS_I2S	   = 8,
	AS_SDM	   = 9,
	AS_DMIC   = 10,
	AS_USB	   = 11,
	AS_SWS	   = 12,
	AS_SWM	   = 13,
	AS_TEST   = 14,
	AS_ADC	   = 15,
	AS_KS     = 16,
	AS_DEBUG  = 17,

    AS_PWM0 	= 20,
    AS_PWM1		= 21,
    AS_PWM2 	= 22,

    AS_PWM0_N	= 26,
    AS_PWM1_N	= 27,
    AS_PWM2_N	= 28,


    AS_32K_CLK_OUTPUT   = 32,
    AS_RESERVE_0   = 33,
    AS_RESERVE_1   = 34,
    AS_RESERVE_2   = 35,
    AS_RESERVE_3   = 36,
    AS_UART_CTS    = 37,
    AS_UART_RTS    = 38,
    AS_UART_TX     = 39,
    AS_UART_RX     = 40,

    AS_I2C_CK      = 41,
    AS_I2C_MCK     = 42,
    AS_I2C_MSD     = 43,
    AS_I2C_SD_OR_SPI_DI  = 44,
    AS_I2C_CK_OR_SPI_CK  = 45,
    AS_I2C_SD            = 46,

    AS_RX_CYC2LNA        = 50,
    AS_SYS_CLK_OUTPUT    = 52,
    AS_TX_CYC2PA         = 53,
    AS_SPI_MCN           = 54,
    AS_SPI_MDO           = 55,
    AS_SPI_MDI           = 56,
    AS_SPI_MCK           = 57,

    AS_SPI_CN            = 58,
    AS_SPI_DO            = 59,
    AS_SPI_DI            = 60,
    AS_SPI_CK            = 61,

    AS_RX_CYC2LNA_OR_SPI_CN = 63,
    AS_TX_CYC2LNA_OR_SPI_DO = 64,
    AS_UART_CTS_OR_SPI_DI   = 65,
    AS_UART_RTS_OR_SPI_CK   = 66,

}GPIO_FuncTypeDef;




/* End of GPIO Alternative Function define  */

#define reg_gpio_in(i)				REG_ADDR8(0x580+((i>>8)<<3))
#define reg_gpio_ie(i)				REG_ADDR8(0x581+((i>>8)<<3))

#define analogRegAddr_gpioPA5_7_ie  0xb6
#define analogRegAddr_gpioPB_ie     0xb9

#define reg_gpio_oen(i)				REG_ADDR8(0x582+((i>>8)<<3))
#define reg_gpio_out(i)				REG_ADDR8(0x583+((i>>8)<<3))
#define reg_gpio_pol(i)				REG_ADDR8(0x584+((i>>8)<<3))

#define reg_gpio_ds(i)				REG_ADDR8(0x585+((i>>8)<<3))
#define analogRegAddr_gpioPA5_7_ds  0xb8
#define analogRegAddr_gpioPB_ds     0xbb

#define reg_gpio_gpio_func(i)		REG_ADDR8(0x586+((i>>8)<<3))
#define reg_gpio_config_func(i)     REG_ADDR16(0x5a8 + ((i>>8)<<1))
//#define reg_gpio_multi_func(i)      REG_ADDR16(0x5a8 + ((i>>8)<<1))

#define reg_gpio_irq_wakeup_en(i)	REG_ADDR8(0x587+((i>>8)<<3))  // reg_irq_mask: FLD_IRQ_GPIO_EN

#define reg_gpio_irq_risc0_en(i)    REG_ADDR8(0x5b8 + (i >> 8))	  // reg_irq_mask: FLD_IRQ_GPIO_RISC0_EN
#define reg_gpio_irq_risc1_en(i)    REG_ADDR8(0x5c0 + (i >> 8))	  // reg_irq_mask: FLD_IRQ_GPIO_RISC1_EN
#define reg_gpio_irq_risc2_en(i)    REG_ADDR8(0x5c8 + (i >> 8))   // reg_irq_mask: FLD_IRQ_GPIO_RISC2_EN

#define reg_gpio_wakeup_and_irq_en  REG_ADDR8(0x5b5)
enum{
    FLD_GPIO_CORE_WAKEUP_EN    = BIT(2),
    FLD_GPIO_CORE_INTERRUPT_EN = BIT(3),
};

/**
 * @brief      This function servers to initialization all gpio.
 * @param[in]  none.
 * @return     none.
 */
void gpio_init(void);

/**
 * @brief      This function servers to set the GPIO's function.
 * @param[in]  pin - the special pin.
 * @param[in]  func - the function of GPIO.
 * @return     none.
 */
void gpio_set_func(GPIO_PinTypeDef pin, GPIO_FuncTypeDef func);

/**
 * @brief      This function set the output function of a pin.
 * @param[in]  pin - the pin needs to set the output function
 * @param[in]  value - enable or disable the pin's output function(0: enable, 1: disable)
 * @return     none
 */
static inline void gpio_set_output_en(GPIO_PinTypeDef pin, unsigned int value)
{
	unsigned char	bit = pin & 0xff;
	if(!value){
		BM_SET(reg_gpio_oen(pin), bit);
	}else{
		BM_CLR(reg_gpio_oen(pin), bit);
	}
}

/**
 * @brief      This function set the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function
 * @param[in]  value - enable or disable the pin's input function(0: disable, 1: enable)
 * @return     none
 */
void gpio_set_input_en(GPIO_PinTypeDef pin, unsigned int value);

/**
 * @brief      This function determines whether the output function of a pin is enabled.
 * @param[in]  pin - the pin needs to determine whether its output function is enabled.
 * @return     1: the pin's output function is enabled ;
 *             0: the pin's output function is disabled
 */
static inline int gpio_is_output_en(GPIO_PinTypeDef pin)
{

	return !BM_IS_SET(reg_gpio_oen(pin), pin & 0xff);
}

/**
 * @brief     This function to judge whether a pin's input is enable.
 * @param[in] pin - the pin needs to enable its input.
 * @return    1:enable the pin's input function.
 *            0:disable the pin's input function.
 */
int  gpio_is_input_en(GPIO_PinTypeDef pin);

/**
 * @brief     This function set the pin's output level.
 * @param[in] pin - the pin needs to set its output level
 * @param[in] value - value of the output level(1: high 0: low)
 * @return    none
 */
static inline void gpio_write(GPIO_PinTypeDef pin, unsigned int value){
	unsigned char	bit = pin & 0xff;
	if(value){
		BM_SET(reg_gpio_out(pin), bit);
	}else{
		BM_CLR(reg_gpio_out(pin), bit);
	}
}

/**
 * @brief     This function read the pin's input/output level.
 * @param[in] pin - the pin needs to read its level
 * @return    the pin's level(1: high 0: low)
 */
static inline unsigned char gpio_read(GPIO_PinTypeDef pin){
	return BM_IS_SET(reg_gpio_in(pin), pin & 0xff);
}

/**
 * @brief     This function set the pin toggle.
 * @param[in] pin - the pin needs to toggle
 * @return    none
 */
static inline void gpio_toggle(GPIO_PinTypeDef pin) {
	reg_gpio_out(pin) ^= (pin & 0xFF);
}

/**
 * @brief      This function set the pin's driving strength.
 * @param[in]  pin - the pin needs to set the driving strength
 * @param[in]  value - the level of driving strength(1: strong 0: poor)
 * @return     none
 */
void gpio_set_data_strength(GPIO_PinTypeDef pin, unsigned int value);

static inline void gpio_core_wakeup_enable_all (int en)
{
    if (en) {
        BM_SET(reg_gpio_wakeup_and_irq_en, FLD_GPIO_CORE_WAKEUP_EN);
    }
    else {
        BM_CLR(reg_gpio_wakeup_and_irq_en, FLD_GPIO_CORE_WAKEUP_EN);
    }
}

/**
 * @brief     This function set a pin's pull-up/down resistor.
 * @param[in] gpio - the pin needs to set its pull-up/down resistor
 * @param[in] up_down - the type of the pull-up/down resistor
 * @return    none
 */
void gpio_setup_up_down_resistor(GPIO_PinTypeDef gpio, GPIO_PullTypeDef up_down);

/**
 * @brief      This function servers to set the specified GPIO as high resistor.
 * @param[in]  pin  - select the specified GPIO
 * @return     none.
 */
void gpio_shutdown(GPIO_PinTypeDef pin);

/**
 * @Brief: 		This function serves to disable or enable 50k pull-up resistor. only PA6,PA7 and PB0~PB7 have
 *        		50K pull-up resistor.
 * @Param[in]: 	en - 0: disable; 1: enable. (disable by default)
 * @Return:		none.
 */
void gpio_set_50k_pullup(unsigned char enable);

/**
 * @brief     This function set a pin's IRQ.
 * @param[in] pin - the pin needs to enable its IRQ
 * @param[in] falling - value of the edge polarity(1: falling edge 0: rising edge)
 * @return    none
 */
static inline void gpio_set_interrupt(GPIO_PinTypeDef pin, unsigned int falling){
	unsigned char	bit = pin & 0xff;
	BM_SET(reg_gpio_irq_wakeup_en(pin), bit);

	reg_irq_mask |= FLD_IRQ_GPIO_EN ; //////refer to driver
	reg_gpio_wakeup_irq |= FLD_GPIO_CORE_INTERRUPT_EN;

	if(falling){
		BM_SET(reg_gpio_pol(pin), bit);
	}else{
		BM_CLR(reg_gpio_pol(pin), bit);
	}
}
/*****
 * brief: this function just enable or disable one or some pins' interrupt function
 * param-pin: GPIO pin
 * param-en:  1:enable; 0:disable
 */
static inline void gpio_en_interrupt(GPIO_PinTypeDef pin, int en){  // reg_irq_mask: FLD_IRQ_GPIO_EN
	unsigned char	bit = pin & 0xff;
	if(en){
		BM_SET(reg_gpio_irq_wakeup_en(pin), bit);
	}
	else{
		BM_CLR(reg_gpio_irq_wakeup_en(pin), bit);
	}
}
/***
 * brief: this function is the master switch(×Ü¿ª¹Ø) of gpio interrupt
 * param-en: 1: enable ; 0: disable
 */
static inline void gpio_core_irq_enable_all (int en)
{
    if (en) {
        BM_SET(reg_gpio_wakeup_and_irq_en, FLD_GPIO_CORE_INTERRUPT_EN);
    }
    else {
        BM_CLR(reg_gpio_wakeup_and_irq_en, FLD_GPIO_CORE_INTERRUPT_EN);
    }
}

/**
 * @brief     This function set a pin's IRQ.
 * @param[in] pin - the pin needs to enable its IRQ
 * @param[in] falling - value of the edge polarity(1: falling edge 0: rising edge)
 * @return    none
 */
static inline void gpio_set_interrupt_risc0(GPIO_PinTypeDef pin, unsigned int falling){
	unsigned char	bit = pin & 0xff;
	BM_SET(reg_gpio_irq_risc0_en(pin), bit);
	reg_irq_mask |= FLD_IRQ_GPIO_RISC0_EN;

	if(falling){
		BM_SET(reg_gpio_pol(pin), bit);
	}else{
		BM_CLR(reg_gpio_pol(pin), bit);
	}
}
/***
 * brief: this function is used to enable or close risc0. user can use it with gpio_set_interrupt_risc0;
 *        gpio_set_interrupt_risc0() to set and enable risc0 interrupt. and gpio_en_interrupt_risc0(0) to close risc0 interrupt.
 */
static inline void gpio_en_interrupt_risc0(GPIO_PinTypeDef pin, int en){  // reg_irq_mask: FLD_IRQ_GPIO_RISC0_EN
	unsigned char	bit = pin & 0xff;
	if(en){
		BM_SET(reg_gpio_irq_risc0_en(pin), bit);
	}
	else{
		BM_CLR(reg_gpio_irq_risc0_en(pin), bit);
	}
}

/**
 * @brief     This function set a pin's IRQ.
 * @param[in] pin - the pin needs to enable its IRQ
 * @param[in] falling - value of the edge polarity(1: falling edge 0: rising edge)
 * @return    none
 */
static inline void gpio_set_interrupt_risc1(GPIO_PinTypeDef pin, unsigned int falling){
	unsigned char	bit = pin & 0xff;
	BM_SET(reg_gpio_irq_risc1_en(pin), bit);
	reg_irq_mask |= FLD_IRQ_GPIO_RISC1_EN;

	if(falling){
		BM_SET(reg_gpio_pol(pin), bit);
	}else{
		BM_CLR(reg_gpio_pol(pin), bit);
	}
}
/***
 * brief: this function is used to enable or close risc1. user can use it with gpio_set_interrupt_risc1;
 *        gpio_set_interrupt_risc1() to set and enable risc1 interrupt. and gpio_en_interrupt_risc1(0) to close risc1 interrupt.
 */
static inline void gpio_en_interrupt_risc1(GPIO_PinTypeDef pin, int en){  // reg_irq_mask: FLD_IRQ_GPIO_RISC1_EN
	unsigned char	bit = pin & 0xff;
	if(en){
		BM_SET(reg_gpio_irq_risc1_en(pin), bit);
	}
	else{
		BM_CLR(reg_gpio_irq_risc1_en(pin), bit);
	}
}

/**
 * @brief     This function set a pin's IRQ.
 * @param[in] pin - the pin needs to enable its IRQ
 * @param[in] falling - value of the edge polarity(1: falling edge 0: rising edge)
 * @return    none
 */
static inline void gpio_set_interrupt_risc2(GPIO_PinTypeDef pin, unsigned int falling){
	unsigned char	bit = pin & 0xff;
	BM_SET(reg_gpio_irq_risc2_en(pin), bit);
	reg_irq_mask |= FLD_IRQ_GPIO_RISC2_EN;

	if(falling){
		BM_SET(reg_gpio_pol(pin), bit);
	}else{
		BM_CLR(reg_gpio_pol(pin), bit);
	}
}

/***
 * brief: this function is used to enable or close risc2. user can use it with gpio_set_interrupt_risc2;
 *        gpio_set_interrupt_risc2() to set and enable risc2 interrupt. and gpio_en_interrupt_risc2(0) to close risc2 interrupt.
 */
static inline void gpio_en_interrupt_risc2(GPIO_PinTypeDef pin, int en){  // reg_irq_mask: FLD_IRQ_GPIO_RISC2_EN
	unsigned char	bit = pin & 0xff;
	if(en){
		BM_SET(reg_gpio_irq_risc2_en(pin), bit);
	}
	else{
		BM_CLR(reg_gpio_irq_risc2_en(pin), bit);
	}
}

//static inline void gpio_set_interrupt_pol(GPIO_PinTypeDef pin, unsigned int falling){
//	unsigned char	bit = pin & 0xff;
//	if(falling){
//		BM_SET(reg_gpio_pol(pin), bit);
//	}else{
//		BM_CLR(reg_gpio_pol(pin), bit);
//	}
//}


static inline unsigned char gpio_read_cache(GPIO_PinTypeDef pin, unsigned char *p){
	return p[pin>>8] & (pin & 0xff);
}

static inline void gpio_read_all(unsigned char *p){
	p[0] = REG_ADDR8(0x580);//PA
	p[1] = REG_ADDR8(0x588);//PB
	p[2] = REG_ADDR8(0x590);//PC
}




void gpio_set_wakeup(GPIO_PinTypeDef pin, unsigned int pol, int en);
void gpio_clear_gpio_irq_flag(void);

