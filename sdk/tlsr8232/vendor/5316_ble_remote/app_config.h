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


/* Function Select -----------------------------------------------------------*/
#define BLE_REMOTE_PM_ENABLE	    	1
#define BLE_REMOTE_SECURITY_ENABLE      1
#define BLE_REMOTE_OTA_ENABLE			1
#define REMOTE_IR_ENABLE				0
#define BATT_CHECK_ENABLE       		1//enable or disable battery voltage detection
#define RC_BTN_ENABLE               	1
#define BLT_APP_LED_ENABLE				1

/* Firmware check ------------------------------------------------------------*/
#define FIRMWARES_SIGNATURE_ENABLE      0

/* Select flash size ---------------------------------------------------------*/
#define FLASH_SIZE_OPTION_128K          0
#define FLASH_SIZE_OPTION_512K          1
#define FLASH_SIZE_OPTION               FLASH_SIZE_OPTION_512K

/* System clock initialization -----------------------------------------------*/
#define CLOCK_SYS_CLOCK_HZ      		16000000
enum{
	CLOCK_SYS_CLOCK_1S  = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};

/* Software timer ------------------------------------------------------------*/
#define BLT_TEST_SOFT_TIMER_ENABLE		0
#if (BLT_TEST_SOFT_TIMER_ENABLE)
	#define BLT_SOFTWARE_TIMER_ENABLE	1
#endif


/* LED -----------------------------------------------------------------------*/
#define	GPIO_LED	GPIO_PB0


/* Matrix Key Configuration --------------------------------------------------*/
#define	MATRIX_ROW_PULL					PM_PIN_PULLDOWN_100K
#define	MATRIX_COL_PULL					PM_PIN_PULLUP_10K

#define	KB_LINE_HIGH_VALID				0   //dirve pin output 0 when keyscan, scanpin read 0 is valid
#define DEEPBACK_FAST_KEYSCAN_ENABLE	1   //proc fast scan when deepsleep back trigged by key press, in case key loss
#define KEYSCAN_IRQ_TRIGGER_MODE		1
#define LONG_PRESS_KEY_POWER_OPTIMIZE	1   //lower power when pressing key without release

//stuck key
#define STUCK_KEY_PROCESS_ENABLE		0
#define STUCK_KEY_ENTERDEEP_TIME		60  //in s

//repeat key
#define KB_REPEAT_KEY_ENABLE			0
#define	KB_REPEAT_KEY_INTERVAL_MS		200
#define KB_REPEAT_KEY_NUM				1
#define KB_MAP_REPEAT					{VK_1, }

/* Key type Macro */
#define IDLE_KEY	   			0
#define CONSUMER_KEY   	   		1
#define KEYBOARD_KEY   	   		2
#define IR_KEY   	   			3

#define			CR_VOL_UP				0xf0  ////
#define			CR_VOL_DN				0xf1
#define			CR_VOL_MUTE				0xf2
#define			CR_CHN_UP				0xf3
#define			CR_CHN_DN				0xf4  ////
#define			CR_POWER				0xf5
#define			CR_SEARCH				0xf6
#define			CR_RECORD				0xf7
#define			CR_PLAY					0xf8  ////
#define			CR_PAUSE				0xf9
#define			CR_STOP					0xfa
#define			CR_FAST_BACKWARD		0xfb
#define			CR_FAST_FORWARD			0xfc  ////
#define			CR_HOME					0xfd
#define			CR_BACK					0xfe
#define			CR_MENU					0xff

//special key
#define		 	VOICE					0xc0
#define 		KEY_MODE_SWITCH			0xc1
#define		 	PHY_TEST				0xc2


#define 		IR_VK_0			0x00
#define 		IR_VK_1			0x01
#define 		IR_VK_2			0x02
#define			IR_VK_3			0x03
#define			IR_VK_4			0x04
#define 		IR_VK_5			0x05
#define 		IR_VK_6			0x06
#define 		IR_VK_7			0x07
#define 		IR_VK_8			0x08
#define 		IR_VK_9			0x09

#define 		IR_POWER		0x12
#define			IR_AUDIO_MUTE	0x0d
#define 		IR_NETFLIX		0x0f
#define			IR_BACK			0x0e
#define			IR_VOL_UP		0x0b
#define			IR_VOL_DN		0x0c
#define 		IR_NEXT			0x20
#define 		IR_PREV			0x21
#define			IR_MENU			0x23
#define 		IR_HOME			0x24
#define 		IR_OPER_KEY		0x2e
#define 		IR_INFO			0x2f
#define			IR_REWIND		0x32
#define 		IR_FAST_FOWARD	0x34
#define 		IR_PLAY_PAUSE	0x35
#define			IR_GUIDE		0x41
#define 		IR_UP			0x45
#define			IR_DN			0x44
#define 		IR_LEFT			0x42
#define 		IR_RIGHT		0x43
#define			IR_SEL			0x46
#define 		IR_RED_KEY		0x6b
#define 		IR_GREEN_KEY	0x6c
#define 		IR_YELLOW_KEY	0x6d
#define 		IR_BLUE_KEY		0x6e
#define 		IR_RECORD		0x72
#define 		IR_OPTION		0x73
#define 		IR_STOP			0x74
#define 		IR_SEARCH		0x75
#define 		IR_TEXT			0x76
#define 		IR_VOICE		0x77
#define 		IR_PAUSE		0x78

#define			T_VK_CH_UP		0xd0
#define			T_VK_CH_DN		0xd1


#if(RC_BTN_ENABLE)
//5316 hardware: C1T125A5_V1.0
#if (REMOTE_IR_ENABLE)  //with IR key map
	#define GPIO_IR_CONTROL	 GPIO_PA0

	#define		KB_MAP_NORMAL	{\
				{0,		           1,	   2,	   3,	  4},  \
				{KEY_MODE_SWITCH,  6,	   7,	   8,     9},  \
				{10,               11,	   12,     13,	  14}, \
				{15,	           16,     17,	   18,	  19}, \
				{20,	           21,	   22,	   23,	  24}, \
				{25,	           26,	   27,	   28,	  29}, }

	#define		KB_MAP_BLE	{\
				VK_NONE,		  VK_UP,	   VK_ENTER,	VK_DOWN,	 VK_NONE,   \
				KEY_MODE_SWITCH,  VK_LEFT,	   CR_MENU,	    CR_VOL_MUTE, VK_RIGHT,  \
				VK_POWER ,        CR_HOME,	   VK_7,   	    VK_2,	     CR_BACK,   \
				VK_NONE,	 	  CR_VOL_DN,   VK_NONE,	    VK_5,	     CR_VOL_UP, \
				VK_NONE,	 	  VK_1,	       VK_0,	    VK_8,	     VK_3,      \
				VK_NONE,		  VK_4,	       VK_NONE,	    VK_9,	     VK_6, }


	#define		KB_MAP_IR	{\
				VK_NONE,	        IR_UP,	    IR_SEL,	    IR_DN,	    VK_NONE,    \
				KEY_MODE_SWITCH,	IR_LEFT,	IR_MENU,	VK_NONE,	IR_RIGHT,	\
				IR_POWER ,	        IR_HOME,	IR_VK_7,	IR_VK_2,	IR_BACK,	\
				VK_NONE,	        IR_VOL_DN,	VK_NONE,	IR_VK_5,    IR_VOL_UP,	\
				VK_NONE,            IR_VK_1,	IR_VK_0,	IR_VK_8,	IR_VK_3,	\
				VK_NONE,	        IR_VK_4,	VK_NONE,	IR_VK_9,	IR_VK_6,  }
#else//key map
	#define		KB_MAP_NORMAL	{\
				{VK_NONE,		  VK_UP,	   VK_ENTER,	VK_DOWN,	 VK_NONE},   \
				{KEY_MODE_SWITCH, VK_LEFT,	   CR_MENU,	    CR_VOL_MUTE, VK_RIGHT},  \
				{VK_POWER,        CR_HOME,	   VK_7,   	    VK_2,	     CR_BACK},   \
				{VK_NONE,	 	  CR_VOL_DN,   VK_NONE,	    VK_5,	     CR_VOL_UP}, \
				{VK_NONE,	 	  VK_1,	       VK_0,	    VK_8,	     VK_3},      \
				{VK_NONE,		  VK_4,	       VK_NONE,	    VK_9,	     VK_6}, }
#endif  //end of REMOTE_IR_ENABLE

#define  KB_DRIVE_PINS  {GPIO_PA5, GPIO_PA4, GPIO_PA3, GPIO_PA2, GPIO_PA1}
#define  KB_SCAN_PINS   {GPIO_PC6, GPIO_PC5, GPIO_PC4, GPIO_PC3, GPIO_PC2, GPIO_PC1}

//drive pin need 100K pulldown
#define	PULL_WAKEUP_SRC_PA5		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PA4		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PA3		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PA2		MATRIX_ROW_PULL
#define	PULL_WAKEUP_SRC_PA1		MATRIX_ROW_PULL

//scan  pin need 10K pullup
#define	PULL_WAKEUP_SRC_PC6		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PC5		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PC4		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PC3		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PC2		MATRIX_COL_PULL
#define	PULL_WAKEUP_SRC_PC1		MATRIX_COL_PULL

//drive pin open input to read gpio wakeup level
#define PA5_INPUT_ENABLE		1
#define PA4_INPUT_ENABLE		1
#define PA3_INPUT_ENABLE		1
#define PA2_INPUT_ENABLE		1
#define PA1_INPUT_ENABLE		1

//scan pin open input to read gpio level
#define PC6_INPUT_ENABLE		1
#define PC5_INPUT_ENABLE		1
#define PC4_INPUT_ENABLE		1
#define PC3_INPUT_ENABLE		1
#define PC2_INPUT_ENABLE		1
#define PC1_INPUT_ENABLE		1

#endif

#define		KB_MAP_NUM		KB_MAP_NORMAL
#define		KB_MAP_FN		KB_MAP_NORMAL

/* WatchDog ------------------------------------------------------------------*/
#define MODULE_WATCHDOG_ENABLE	0
#define WATCHDOG_INIT_TIMEOUT	500  //Unit:ms

/* ATT Handle define ---------------------------------------------------------*/
typedef enum
{
	ATT_H_START = 0,


	//// Gap ////
	/**********************************************************************************************/
	GenericAccess_PS_H, 					//UUID: 2800, 	VALUE: uuid 1800
	GenericAccess_DeviceName_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read | Notify
	GenericAccess_DeviceName_DP_H,			//UUID: 2A00,   VALUE: device name
	GenericAccess_Appearance_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	GenericAccess_Appearance_DP_H,			//UUID: 2A01,	VALUE: appearance
	CONN_PARAM_CD_H,						//UUID: 2803, 	VALUE:  			Prop: Read
	CONN_PARAM_DP_H,						//UUID: 2A04,   VALUE: connParameter

	
	//// gatt ////
	/**********************************************************************************************/
	GenericAttribute_PS_H,					//UUID: 2800, 	VALUE: uuid 1801
	GenericAttribute_ServiceChanged_CD_H,	//UUID: 2803, 	VALUE:  			Prop: Indicate
	GenericAttribute_ServiceChanged_DP_H,   //UUID:	2A05,	VALUE: service change
	GenericAttribute_ServiceChanged_CCB_H,	//UUID: 2902,	VALUE: serviceChangeCCC
	

	//// device information ////
	/**********************************************************************************************/
	DeviceInformation_PS_H,					//UUID: 2800, 	VALUE: uuid 180A
	DeviceInformation_pnpID_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	DeviceInformation_pnpID_DP_H,			//UUID: 2A50,	VALUE: PnPtrs
	

	//// HID ////
	/**********************************************************************************************/
	HID_PS_H, 								//UUID: 2800, 	VALUE: uuid 1812

	//include
	HID_INCLUDE_H,							//UUID: 2802, 	VALUE: include

	//protocol
	HID_PROTOCOL_MODE_CD_H,					//UUID: 2803, 	VALUE:  			Prop: read | write_without_rsp	
	HID_PROTOCOL_MODE_DP_H,					//UUID: 2A4E,	VALUE: protocolMode
	
	//boot keyboard input report
	HID_BOOT_KB_REPORT_INPUT_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read | Notify
	HID_BOOT_KB_REPORT_INPUT_DP_H,			//UUID: 2A22, 	VALUE: bootKeyInReport
	HID_BOOT_KB_REPORT_INPUT_CCB_H,			//UUID: 2902, 	VALUE: bootKeyInReportCCC

	//boot keyboard output report
	HID_BOOT_KB_REPORT_OUTPUT_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read | write| write_without_rsp
	HID_BOOT_KB_REPORT_OUTPUT_DP_H,			//UUID: 2A32, 	VALUE: bootKeyOutReport

	//consume report in
	HID_CONSUME_REPORT_INPUT_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read | Notify
	HID_CONSUME_REPORT_INPUT_DP_H,			//UUID: 2A4D, 	VALUE: reportConsumerIn
	HID_CONSUME_REPORT_INPUT_CCB_H,			//UUID: 2902, 	VALUE: reportConsumerInCCC
	HID_CONSUME_REPORT_INPUT_REF_H, 		//UUID: 2908    VALUE: REPORT_ID_CONSUMER, TYPE_INPUT
	
	//keyboard report in
	HID_NORMAL_KB_REPORT_INPUT_CD_H,		//UUID: 2803, 	VALUE:  			Prop: Read | Notify
	HID_NORMAL_KB_REPORT_INPUT_DP_H,		//UUID: 2A4D, 	VALUE: reportKeyIn
	HID_NORMAL_KB_REPORT_INPUT_CCB_H,		//UUID: 2902, 	VALUE: reportKeyInInCCC
	HID_NORMAL_KB_REPORT_INPUT_REF_H, 		//UUID: 2908    VALUE: REPORT_ID_KEYBOARD, TYPE_INPUT

	//keyboard report out
	HID_NORMAL_KB_REPORT_OUTPUT_CD_H,		//UUID: 2803, 	VALUE:  			Prop: Read | write| write_without_rsp
	HID_NORMAL_KB_REPORT_OUTPUT_DP_H,  		//UUID: 2A4D, 	VALUE: reportKeyOut
	HID_NORMAL_KB_REPORT_OUTPUT_REF_H, 		//UUID: 2908    VALUE: REPORT_ID_KEYBOARD, TYPE_OUTPUT
	
	// report map
	HID_REPORT_MAP_CD_H,					//UUID: 2803, 	VALUE:  			Prop: Read
	HID_REPORT_MAP_DP_H,					//UUID: 2A4B, 	VALUE: reportKeyIn
	HID_REPORT_MAP_EXT_REF_H,				//UUID: 2907 	VALUE: extService

	//hid information
	HID_INFORMATION_CD_H,					//UUID: 2803, 	VALUE:  			Prop: read
	HID_INFORMATION_DP_H,					//UUID: 2A4A 	VALUE: hidInformation
	
	//control point
	HID_CONTROL_POINT_CD_H,					//UUID: 2803, 	VALUE:  			Prop: write_without_rsp
	HID_CONTROL_POINT_DP_H,					//UUID: 2A4C 	VALUE: controlPoint


	//// battery service ////
	/**********************************************************************************************/
	BATT_PS_H, 								//UUID: 2800, 	VALUE: uuid 180f
	BATT_LEVEL_INPUT_CD_H,					//UUID: 2803, 	VALUE:  			Prop: Read | Notify
	BATT_LEVEL_INPUT_DP_H,					//UUID: 2A19 	VALUE: batVal
	BATT_LEVEL_INPUT_CCB_H,					//UUID: 2902, 	VALUE: batValCCC


	//// Ota ////
	/**********************************************************************************************/
	OTA_PS_H, 								//UUID: 2800, 	VALUE: telink ota service uuid
	OTA_CMD_OUT_CD_H,						//UUID: 2803, 	VALUE:  			Prop: read | write_without_rsp	
	OTA_CMD_OUT_DP_H,						//UUID: telink ota uuid,  VALUE: otaData
	OTA_CMD_OUT_DESC_H,						//UUID: 2901, 	VALUE: otaName


	
	ATT_END_H,

}ATT_HANDLE;

















/* Simulate uart debug Interface ---------------------------------------------*/
#define SIMULATE_UART_EN       0
#define DEBUG_TX_PIN           GPIO_PB4
#define DEBUG_BAUDRATE         (115200)


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
