/********************************************************************************************************
 * @file     app_config.h
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
 * @date     May. 12, 2018
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *			 The information contained herein is confidential and proprietary property of Telink
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in.
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#define RC_BTN_ENABLE               	1

//////////////////////// TEST FEATURE SELECTION ///////////////////////////////

#define TEST_HW_TIMER                    1
#define TEST_GPIO_IRQ                   10
#define TEST_UART                       20
#define TEST_IIC                        30
#define TEST_SPI                        40
#define TEST_ADC                        50
#define TEST_PWM                        60
#define TEST_LOW_POWER                  70
#define TEST_RF_EMI                     80


#define DRIVER_TEST_MODE                TEST_RF_EMI





/* LED -----------------------------------------------------------------------*/
#define	GPIO_LED	GPIO_PB0



/* System clock initialization -----------------------------------------------*/
//#define INTERNAL_RC     0
//#define EXTERNANL_XTAL  1
//#define CLOCK_SRC               EXTERNANL_XTAL

#define CLOCK_SYS_CLOCK_HZ      16000000
enum{
	CLOCK_SYS_CLOCK_1S  = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};


/* WatchDog ------------------------------------------------------------------*/
#define MODULE_WATCHDOG_ENABLE	0
#define WATCHDOG_INIT_TIMEOUT	500  //Unit:ms




/* Debug Interface -----------------------------------------------------------*/
#define  DEBUG_GPIO_ENABLE					0

#if(DEBUG_GPIO_ENABLE)
		#define PB2_FUNC				AS_GPIO //debug gpio chn0 : PB2
		#define PB3_FUNC				AS_GPIO //debug gpio chn1 : PB3
		#define PB4_FUNC				AS_GPIO //debug gpio chn2 : PB4
		#define PB5_FUNC				AS_GPIO //debug gpio chn3 : PB5
		#define PA6_FUNC                AS_GPIO //debug gpio chn4 : PA6
		#define PA7_FUNC                AS_GPIO //debug gpio chn5 : PA7

		#define PB2_INPUT_ENABLE					0
		#define PB3_INPUT_ENABLE					0
		#define PB4_INPUT_ENABLE					0
		#define PB5_INPUT_ENABLE					0
		#define PA6_INPUT_ENABLE					0
		#define PA7_INPUT_ENABLE					0

		#define PB2_OUTPUT_ENABLE					1
		#define PB3_OUTPUT_ENABLE					1
		#define PB4_OUTPUT_ENABLE					1
		#define PB5_OUTPUT_ENABLE					1
		#define PA6_OUTPUT_ENABLE					1
		#define PA7_OUTPUT_ENABLE					1

		#define DBG_CHN0_LOW		( *(unsigned char *)0x80058b &= (~(1<<2)) )
		#define DBG_CHN0_HIGH		( *(unsigned char *)0x80058b |= (1<<2) )
		#define DBG_CHN0_TOGGLE		( *(unsigned char *)0x80058b ^= (1<<2) )

		#define DBG_CHN1_LOW		( *(unsigned char *)0x80058b &= (~(1<<3)) )
		#define DBG_CHN1_HIGH		( *(unsigned char *)0x80058b |= (1<<3) )
		#define DBG_CHN1_TOGGLE		( *(unsigned char *)0x80058b ^= (1<<3) )

		#define DBG_CHN2_LOW		( *(unsigned char *)0x80058b &= (~(1<<4)) )
		#define DBG_CHN2_HIGH		( *(unsigned char *)0x80058b |= (1<<4) )
		#define DBG_CHN2_TOGGLE		( *(unsigned char *)0x80058b ^= (1<<4) )

		#define DBG_CHN3_LOW		( *(unsigned char *)0x80058b &= (~(1<<5)) )
		#define DBG_CHN3_HIGH		( *(unsigned char *)0x80058b |= (1<<5) )
		#define DBG_CHN3_TOGGLE		( *(unsigned char *)0x80058b ^= (1<<5) )

		#define DBG_CHN4_LOW		( *(unsigned char *)0x800583 &= (~(1<<6)) )
		#define DBG_CHN4_HIGH		( *(unsigned char *)0x800583 |= (1<<6) )
		#define DBG_CHN4_TOGGLE		( *(unsigned char *)0x800583 ^= (1<<6) )

		#define DBG_CHN5_LOW		( *(unsigned char *)0x800583 &= (~(1<<7)) )
		#define DBG_CHN5_HIGH		( *(unsigned char *)0x800583 |= (1<<7) )
		#define DBG_CHN5_TOGGLE		( *(unsigned char *)0x800583 ^= (1<<7) )
#else
		#define DBG_CHN0_LOW
		#define DBG_CHN0_HIGH
		#define DBG_CHN0_TOGGLE
		#define DBG_CHN1_LOW
		#define DBG_CHN1_HIGH
		#define DBG_CHN1_TOGGLE
		#define DBG_CHN2_LOW
		#define DBG_CHN2_HIGH
		#define DBG_CHN2_TOGGLE
		#define DBG_CHN3_LOW
		#define DBG_CHN3_HIGH
		#define DBG_CHN3_TOGGLE
		#define DBG_CHN4_LOW
		#define DBG_CHN4_HIGH
		#define DBG_CHN4_TOGGLE
		#define DBG_CHN5_LOW
		#define DBG_CHN5_HIGH
		#define DBG_CHN5_TOGGLE

#endif  //end of DEBUG_GPIO_ENABLE



#include "../common/default_config.h"

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
