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




/////////////////////HCI ACCESS OPTIONS/////////////////////
#define HCI_USE_UART	1
#define HCI_USE_USB		0
#define HCI_ACCESS		HCI_USE_UART

/* Function Select -----------------------------------------------------------*/
#define BLE_HCI_PM_ENABLE			    0


/***select flash size***/
#define FLASH_SIZE_OPTION_128K          0
#define FLASH_SIZE_OPTION_512K          1

#define FLASH_SIZE_OPTION               FLASH_SIZE_OPTION_512K


/* LED -----------------------------------------------------------------------*/
#define	GPIO_LED	GPIO_PB0


//////////////////////////// MODULE PM GPIO	/////////////////////////////////
#define GPIO_WAKEUP_MODULE					GPIO_PB2   //mcu wakeup module
#define	PB2_FUNC							AS_GPIO
#define PB2_INPUT_ENABLE					1
#define	PB2_OUTPUT_ENABLE					0
#define	PB2_DATA_OUT						0
#define GPIO_WAKEUP_MODULE_HIGH				gpio_setup_up_down_resistor(GPIO_WAKEUP_MODULE, PM_PIN_PULLUP_10K);
#define GPIO_WAKEUP_MODULE_LOW				gpio_setup_up_down_resistor(GPIO_WAKEUP_MODULE, PM_PIN_PULLDOWN_100K);

#define GPIO_WAKEUP_MCU						GPIO_PB3   //module wakeup mcu
#define	PB3_FUNC							AS_GPIO
#define PB3_INPUT_ENABLE					1
#define	PB3_OUTPUT_ENABLE					1
#define	PB3_DATA_OUT						0
#define GPIO_WAKEUP_MCU_HIGH				do{gpio_set_output_en(GPIO_WAKEUP_MCU, 1); gpio_write(GPIO_WAKEUP_MCU, 1);}while(0)
#define GPIO_WAKEUP_MCU_LOW					do{gpio_set_output_en(GPIO_WAKEUP_MCU, 1); gpio_write(GPIO_WAKEUP_MCU, 0);}while(0)
#define GPIO_WAKEUP_MCU_FLOAT				do{gpio_set_output_en(GPIO_WAKEUP_MCU, 0); gpio_write(GPIO_WAKEUP_MCU, 0);}while(0)


/* System clock initialization -----------------------------------------------*/
#define CLOCK_SYS_CLOCK_HZ      16000000
enum{
	CLOCK_SYS_CLOCK_1S  = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};





/* Debug Interface -----------------------------------------------------------*/
#define  DEBUG_GPIO_ENABLE					0

#if(DEBUG_GPIO_ENABLE)
		#define PB2_FUNC				AS_GPIO //debug gpio chn0 : PB2
		#define PB3_FUNC				AS_GPIO //debug gpio chn1 : PB3
		#define PB4_FUNC				AS_GPIO //debug gpio chn2 : PB4
		#define PB5_FUNC				AS_GPIO //debug gpio chn3 : PB5
		#define PA6_FUNC                AS_GPIO //debug gpio chn4 : PA6

		#define PB2_INPUT_ENABLE					0
		#define PB3_INPUT_ENABLE					0
		#define PB4_INPUT_ENABLE					0
		#define PB5_INPUT_ENABLE					0
		#define PA6_INPUT_ENABLE					0

		#define PB2_OUTPUT_ENABLE					1
		#define PB3_OUTPUT_ENABLE					1
		#define PB4_OUTPUT_ENABLE					1
		#define PB5_OUTPUT_ENABLE					1
		#define PA6_OUTPUT_ENABLE					1


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










/////////////////// set default   ////////////////

#include "../common/default_config.h"

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
