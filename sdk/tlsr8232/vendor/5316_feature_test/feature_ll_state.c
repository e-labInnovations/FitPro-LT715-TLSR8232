/********************************************************************************************************
 * @file     feature_ll_state.c 
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     May. 10, 2018
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
#include "app.h"
#include <stack/ble/ble.h>
#include "app_config.h"
#include "vendor/common/blt_led.h"
#include "vendor/common/blt_soft_timer.h"
#include "vendor/common/blt_common.h"

#if(  FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY \
    ||FEATURE_TEST_MODE == TEST_ADV_IN_CONN_SLAVE_ROLE \
    ||FEATURE_TEST_MODE == TEST_SCAN_IN_ADV_AND_CONN_SLAVE_ROLE \
    ||FEATURE_TEST_MODE == TEST_ADV_SCAN_IN_CONN_SLAVE_ROLE)

#define BLE_PM_ENABLE     0

#define	MY_RF_POWER_INDEX			RF_POWER_7P9dBm

#define RX_FIFO_SIZE	64
#define RX_FIFO_NUM		8

#define TX_FIFO_SIZE	40
#define TX_FIFO_NUM		16

u8 blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
my_fifo_t blt_rxfifo = {
		RX_FIFO_SIZE,
		RX_FIFO_NUM,
		0,
		0,
		blt_rxfifo_b,
};

u8 blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
my_fifo_t blt_txfifo = {
	TX_FIFO_SIZE,
	TX_FIFO_NUM,
	0,
	0,
	blt_txfifo_b,
};

void task_connect(u8 e, u8 *p, int n)
{
	//bls_l2cap_requestConnParamUpdate(8,8,99,400);
}

void task_terminate(u8 e, u8 *p, int n)
{

}

void func_suspend_enter(u8 e, u8 *p, int n)
{

}

_attribute_ram_code_ void func_suspend_exit(u8 e, u8 *p, int n)
{
	rf_set_power_level_index(MY_RF_POWER_INDEX);
}

int hci_event_handle(u32 h, u8 *para, int n)
{
	u8 evCode = h & 0xff;
	if(evCode == HCI_EVT_LE_META){
		event_adv_report_t *p = (event_adv_report_t*)para;
		u8 subEventCode = p->subcode;
		if(subEventCode == HCI_SUB_EVT_LE_ADVERTISING_REPORT){
			print_array(p->mac, 6);
		}
	}
	return 0;
}


void feature_linklayer_state_test_init(void)
{
	u8  mac_public[6];
	u8  mac_random_static[6];
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

	rf_set_power_level_index(MY_RF_POWER_INDEX);

	////// Controller Initialization  //////////
	blc_ll_initBasicMCU(mac_public);   //mandatory

#if(FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY)

	printf("\n\rtst adv only\n");
	blc_ll_initAdvertising_module(mac_public);


	u8 tbl_advData[] = {
		0x08, 0x09, 't', 'e', 's', 't', 'a', 'd', 'v',
		0x02, 0x01, 0x05,
	};
	u8	tbl_scanRsp [] = {
		0x08, 0x09, 'T', 'E', 'S', 'T', 'A', 'D', 'V',	//scan name
	};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_100MS, ADV_INTERVAL_100MS, \
									ADV_TYPE_NONCONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);


	if(status != BLE_SUCCESS){     //adv setting err
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}

	blc_ll_setAdvCustomedChannel(37, 38, 39);
	bls_ll_setAdvEnable(1);  //adv enable

#elif(FEATURE_TEST_MODE == TEST_ADV_IN_CONN_SLAVE_ROLE)
	/*-- BLE Controller initialization ---------------------------------------*/
	//blc_ll_initBasicMCU(mac_public);//mandatory
	blc_ll_initAdvertising_module(mac_public);//adv module: mandatory for BLE slave,
	blc_ll_initSlaveRole_module();//slave module: mandatory for BLE slave,

	/*-- BLE Host initialization ---------------------------------------------*/
	extern void my_att_init(void);
	//GATT initialization
	my_att_init();
	//L2CAP initialization
	blc_l2cap_register_handler(blc_l2cap_packet_receive);

	/*-- BLE SMP initialization ----------------------------------------------*/
  #if(BLE_REMOTE_SECURITY_ENABLE)
	blc_smp_param_setBondingDeviceMaxNumber(4);  	//default is SMP_BONDING_DEVICE_MAX_NUM, can not bigger that this value
													//and this func must call before bls_smp_enableParing
	bls_smp_enableParing(SMP_PARING_CONN_TRRIGER );
  #else
	bls_smp_enableParing(SMP_PARING_DISABLE_TRRIGER );
  #endif

	/*-- USER application initialization -------------------------------------*/
	u8 tbl_advData[] = {
		0x09, 0x09, 's', 'l', 'a', 'v', 'e', 'a', 'd', 'v',
	    0x02, 0x01, 0x05,
	};
	u8 tbl_scanRsp[] = {
		0x09, 0x09, 'S', 'L', 'A', 'V', 'E', 'A', 'D', 'V',
	};
	bls_ll_setAdvData((u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData((u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS,
									ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
									0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);
	if(status != BLE_SUCCESS){
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}

	bls_ll_setAdvEnable(1);
	rf_set_power_level_index(RF_POWER_7P9dBm);

	//ADV data in connection state
	u8 tbl_advData_test[] = {
		 0x09, 0x09, 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
		 0x02, 0x01, 0x05,
	};
	u8 tbl_scanRsp_test[] = {
		 0x09, 0x09, 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B',
	};
	blc_ll_addAdvertisingInConnSlaveRole();
	blc_ll_setAdvParamInConnSlaveRole((u8 *)tbl_advData_test, sizeof(tbl_advData_test),
			                          (u8 *)tbl_scanRsp_test, sizeof(tbl_scanRsp_test),
			                          ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
			                          BLT_ENABLE_ADV_ALL,
			                          ADV_FP_NONE);

	//ble event call back
	bls_app_registerEventCallback(BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback(BLT_EV_FLAG_TERMINATE, &task_terminate);

#elif(FEATURE_TEST_MODE == TEST_SCAN_IN_ADV_AND_CONN_SLAVE_ROLE)
	/*-- BLE Controller initialization ---------------------------------------*/
	//blc_ll_initBasicMCU(mac_public);//mandatory
	blc_ll_initAdvertising_module(mac_public);//adv module: mandatory for BLE slave,
	blc_ll_initSlaveRole_module();//slave module: mandatory for BLE slave,

	/*-- BLE Host initialization ---------------------------------------------*/
	extern void my_att_init(void);
	//GATT initialization
	my_att_init();
	//L2CAP initialization
	blc_l2cap_register_handler(blc_l2cap_packet_receive);

	/*-- BLE SMP initialization ----------------------------------------------*/
  #if(BLE_REMOTE_SECURITY_ENABLE)
	blc_smp_param_setBondingDeviceMaxNumber(4);  	//default is SMP_BONDING_DEVICE_MAX_NUM, can not bigger that this value
													//and this func must call before bls_smp_enableParing
	bls_smp_enableParing(SMP_PARING_CONN_TRRIGER );
  #else
	bls_smp_enableParing(SMP_PARING_DISABLE_TRRIGER );
  #endif

	/*-- USER application initialization -------------------------------------*/
	u8 tbl_advData[] = {
		0x09, 0x09, 's', 'l', 'a', 'v', 'e', 'a', 'd', 'v',
	    0x02, 0x01, 0x05,
	};
	u8 tbl_scanRsp[] = {
		0x09, 0x09, 'S', 'L', 'A', 'V', 'E', 'A', 'D', 'V',
	};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS,
									ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
									0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);
	if(status != BLE_SUCCESS){
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}

	bls_ll_setAdvEnable(1);
	rf_set_power_level_index(RF_POWER_7P9dBm);

	//Scan
	blc_ll_initScanning_module(mac_public);
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(&hci_event_handle);

  #if 1  //report all adv
	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE,  SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
							OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
  #else //report adv only in whitelist
	ll_whiteList_reset();
	u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
	ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
							 OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);
  #endif
    blc_ll_addScanningInAdvState();  //add scan in adv state
    blc_ll_addScanningInConnSlaveRole();  //add scan in conn slave role

	//ble event call back
	bls_app_registerEventCallback(BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback(BLT_EV_FLAG_TERMINATE, &task_terminate);

	uart_set_pin(UART_TX_PB4,UART_RX_PB5);
	uart_init_baudrate(9,13,PARITY_NONE, STOP_BIT_ONE);
#elif(FEATURE_TEST_MODE == TEST_ADV_SCAN_IN_CONN_SLAVE_ROLE)
	/*-- BLE Controller initialization ---------------------------------------*/
	//blc_ll_initBasicMCU(mac_public);//mandatory
	blc_ll_initAdvertising_module(mac_public);//adv module: mandatory for BLE slave,
	blc_ll_initSlaveRole_module();//slave module: mandatory for BLE slave,

	/*-- BLE Host initialization ---------------------------------------------*/
	extern void my_att_init(void);
	//GATT initialization
	my_att_init();
	//L2CAP initialization
	blc_l2cap_register_handler(blc_l2cap_packet_receive);

	/*-- BLE SMP initialization ----------------------------------------------*/
  #if(BLE_REMOTE_SECURITY_ENABLE)
	blc_smp_param_setBondingDeviceMaxNumber(4);  	//default is SMP_BONDING_DEVICE_MAX_NUM, can not bigger that this value
													//and this func must call before bls_smp_enableParing
	bls_smp_enableParing(SMP_PARING_CONN_TRRIGER );
  #else
	bls_smp_enableParing(SMP_PARING_DISABLE_TRRIGER );
  #endif

	/*-- USER application initialization -------------------------------------*/
	u8 tbl_advData[] = {
		0x09, 0x09, 's', 'l', 'a', 'v', 'e', 'a', 'd', 'v',
	    0x02, 0x01, 0x05,
	};
	u8 tbl_scanRsp[] = {
		0x09, 0x09, 'S', 'L', 'A', 'V', 'E', 'A', 'D', 'V',
	};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS,
									ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
									0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);
	if(status != BLE_SUCCESS){
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}

	bls_ll_setAdvEnable(1);
	rf_set_power_level_index(RF_POWER_7P9dBm);

	//ADV data in connection state
	u8 tbl_advData_test[] = {
		 0x09, 0x09, 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
		 0x02, 0x01, 0x05,
	};
	u8 tbl_scanRsp_test[] = {
		 0x09, 0x09, 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B',
	};
	blc_ll_addAdvertisingInConnSlaveRole();
	blc_ll_setAdvParamInConnSlaveRole((u8 *)tbl_advData_test, sizeof(tbl_advData_test),
			                          (u8 *)tbl_scanRsp_test, sizeof(tbl_scanRsp_test),
			                          ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
			                          BLT_ENABLE_ADV_ALL,
			                          ADV_FP_NONE);

	//Scan
	blc_ll_initScanning_module(mac_public);
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(&hci_event_handle);

   #if 1  //report all adv
	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE,  SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
							OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
   #else //report adv only in whitelist
	ll_whiteList_reset();
	u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
	ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
							 OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);
   #endif

	blc_ll_addScanningInConnSlaveRole();  //add scan in conn slave role

	//ble event call back
	bls_app_registerEventCallback(BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback(BLT_EV_FLAG_TERMINATE, &task_terminate);

	uart_set_pin(UART_TX_PB4,UART_RX_PB5);
	uart_init_baudrate(9,13,PARITY_NONE, STOP_BIT_ONE);
#endif


#if(BLE_PM_ENABLE)
	blc_ll_initPowerManagement_module();
	bls_pm_setSuspendMask(SUSPEND_ADV | SUSPEND_CONN);

	//bls_app_registerEventCallback(BLT_EV_FLAG_SUSPEND_ENTER, &func_suspend_enter);
	//bls_app_registerEventCallback(BLT_EV_FLAG_SUSPEND_EXIT, &func_suspend_exit);
#else
	bls_pm_setSuspendMask(SUSPEND_DISABLE);
#endif
}


static u8 test_data[] = {1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0};
void feature_linklayer_state_test_main_loop(void)
{
	if(BLE_SUCCESS == bls_att_pushNotifyData(0x0015, test_data, sizeof(test_data))){
		test_data[0]++;
	}
}


#endif


