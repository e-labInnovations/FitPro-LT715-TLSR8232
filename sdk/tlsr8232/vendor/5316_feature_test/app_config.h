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


#define BLE_REMOTE_SECURITY_ENABLE      1

/***select flash size***/
#define FLASH_SIZE_OPTION_128K          0
#define FLASH_SIZE_OPTION_512K          1

#define FLASH_SIZE_OPTION               FLASH_SIZE_OPTION_512K

/////////////////// TEST FEATURE SELECTION /////////////////////////////////

//ble link layer test
#define	TEST_ADVERTISING_ONLY							1
//text extend state machine
#define TEST_ADV_IN_CONN_SLAVE_ROLE                     2
#define TEST_SCAN_IN_ADV_AND_CONN_SLAVE_ROLE            3
#define TEST_ADV_SCAN_IN_CONN_SLAVE_ROLE	            4

//power test
#define TEST_POWER_ADV									10

//smp test
#define TEST_SMP_PASSKEY_ENTRY							20

//gatt secure test
#define TEST_GATT_SECURITY								21 //If testing SECURITY, such as Passkey Entry or Numric_Comparison, we use the remote control board for testing

//data length exchange test   TEST_SDATA_LENGTH_EXTENSION
#define TEST_DATA_LENGTH_EXTENSION						22



//other test
#define TEST_USER_BLT_SOFT_TIMER						30
#define TEST_WHITELIST									31
//phy test
#define TEST_BLE_PHY									32

#define TEST_2M_PHY_CONNECTION					        40

#define FEATURE_TEST_MODE								TEST_DATA_LENGTH_EXTENSION


#if (FEATURE_TEST_MODE == TEST_USER_BLT_SOFT_TIMER)
	#define BLT_SOFTWARE_TIMER_ENABLE					1
#endif



/////////////////////HCI ACCESS OPTIONS///////////////////////////////////////

#define 		PHYTEST_MODE_THROUGH_2_WIRE_UART		1   //Direct Test Mode through a 2-wire UART interface
#define 		PHYTEST_MODE_OVER_HCI_WITH_UART			2   //Direct Test Mode over HCI(USB  hardware interface)

#if (FEATURE_TEST_MODE == TEST_BLE_PHY)
	#define BLE_PHYTEST_MODE					     PHYTEST_MODE_OVER_HCI_WITH_UART
#endif

/////////////////// Clock  /////////////////////////////////
#define CLOCK_SYS_CLOCK_HZ  	16000000
enum{
	CLOCK_SYS_CLOCK_1S  = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};


///////////////////////////////////// ATT  HANDLER define ///////////////////////////////////////
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
	DeviceInformation_PS_H,					 //UUID: 2800, 	VALUE: uuid 180A
	DeviceInformation_pnpID_CD_H,			 //UUID: 2803, 	VALUE:  			Prop: Read
	DeviceInformation_pnpID_DP_H,			 //UUID: 2A50,	VALUE: PnPtrs

  #if(FEATURE_TEST_MODE == TEST_DATA_LENGTH_EXTENSION)
	//// SPP ////
	/**********************************************************************************************/
	SPP_PS_H, 							 //UUID: 2800, 	VALUE: telink spp service uuid

	//server to client
	SPP_SERVER_TO_CLIENT_CD_H,		     //UUID: 2803, 	VALUE:  			Prop: read | Notify
	SPP_SERVER_TO_CLIENT_DP_H,			 //UUID: telink spp s2c uuid,  VALUE: SppDataServer2ClientData
	SPP_SERVER_TO_CLIENT_CCB_H,			 //UUID: 2902, 	VALUE: SppDataServer2ClientDataCCC
	SPP_SERVER_TO_CLIENT_DESC_H,		 //UUID: 2901, 	VALUE: TelinkSPPS2CDescriptor

	//client to server
	SPP_CLIENT_TO_SERVER_CD_H,		     //UUID: 2803, 	VALUE:  			Prop: read | write_without_rsp
	SPP_CLIENT_TO_SERVER_DP_H,			 //UUID: telink spp c2s uuid,  VALUE: SppDataClient2ServerData
	SPP_CLIENT_TO_SERVER_DESC_H,		 //UUID: 2901, 	VALUE: TelinkSPPC2SDescriptor

	//// Ota ////
	/**********************************************************************************************/
	OTA_PS_H, 							 //UUID: 2800, 	VALUE: telink ota service uuid
	OTA_CMD_OUT_CD_H,					 //UUID: 2803, 	VALUE:  			Prop: read | write_without_rsp
	OTA_CMD_OUT_DP_H,					 //UUID: telink ota uuid,  VALUE: otaData
	OTA_CMD_OUT_DESC_H,					 //UUID: 2901, 	VALUE: otaName
  #else

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
	HID_BOOT_KB_REPORT_OUTPUT_DP_H,		    //UUID: 2A32, 	VALUE: bootKeyOutReport

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
   #endif

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
        #define PA7_FUNC                AS_GPIO //debug gpio chn4 : PA7

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

