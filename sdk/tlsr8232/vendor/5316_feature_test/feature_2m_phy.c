/********************************************************************************************************
 * @file     feature_2m_phy_conn.c
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
#include "stack/ble/ble.h"
#include "tl_common.h"
#include "drivers.h"
#include "vendor/common/blt_common.h"


#if(FEATURE_TEST_MODE == TEST_2M_PHY_CONNECTION)

#define BLE_PM_ENABLE     1

MYFIFO_INIT(blt_rxfifo, 64, 8);
MYFIFO_INIT(blt_txfifo, 40, 16);


/* ADV Packet, SCAN Response Packet define */
const u8 tbl_advData[] = {
	 0x05, 0x09, 'G', 'h', 'i', 'd',
	 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
	 0x03, 0x19, 0x80, 0x01, 					// 384, Generic Remote Control, Generic category
	 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		// incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8 tbl_scanRsp [] = {
	0x08, 0x09, 'G', 'R', 'e', 'm', 'o', 't', 'e',
};

int device_in_connection_state;
u32 device_connection_tick;
u32	advertise_begin_tick;

void task_connect(u8 e, u8 *p, int n)
{
	//bls_l2cap_requestConnParamUpdate (8, 8, 99, 400);  //interval=10ms latency=99 timeout=4s
	device_in_connection_state = 1;//

	device_connection_tick = clock_time() | 1; //none zero
}

void task_terminate(u8 e,u8 *p, int n) //*p is terminate reason
{
	device_in_connection_state = 0;
	device_connection_tick = 0;
}

void callback_phy_update_complete_event(u8 e,u8 *p, int n)
{
//	hci_le_phyUpdateCompleteEvt_t *pEvt = (hci_le_phyUpdateCompleteEvt_t *)p;

//	DBG_CHN0_TOGGLE;

	DBG_CHN5_TOGGLE;
}

void feature_2m_phy_conn_init(void)
{
	/*-- BLE stack initialization --------------------------------------------*/
	u8  mac_public[6];
	u8  mac_random_static[6];
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

	/*-- BLE Controller initialization ---------------------------------------*/
	blc_ll_initBasicMCU(mac_public);//mandatory
	blc_ll_initAdvertising_module(mac_public);//adv module: mandatory for BLE slave,
	blc_ll_initSlaveRole_module();//slave module: mandatory for BLE slave,

	blc_ll_init2MPhy_feature();

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
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER );
#else
	bls_smp_enableParing (SMP_PARING_DISABLE_TRRIGER );
#endif

	/*-- USER application initialization -------------------------------------*/
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	/* Configure ADV packet */
	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_50MS, ADV_INTERVAL_50MS,
									ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
									0,  NULL,
									BLT_ENABLE_ADV_ALL,
									ADV_FP_NONE);
	//debug: ADV setting err
	if(status != BLE_SUCCESS) { write_reg8(0x8000, 0x11); 	while(1); }

	bls_ll_setAdvEnable(1);  //adv enable
	rf_set_power_level_index (RF_POWER_7P9dBm);//OK

	//ble event call back
	bls_app_registerEventCallback(BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback(BLT_EV_FLAG_TERMINATE, &task_terminate);
	bls_app_registerEventCallback(BLT_EV_FLAG_PHY_UPDATE, &callback_phy_update_complete_event);

	/* Power Management initialization */
#if(BLE_PM_ENABLE)
	blc_ll_initPowerManagement_module();        //pm module:      	 optional
	bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);
#endif
}


// main loop flow
/////////////////////////////////////////////////////////////////////
u32 tick_loop;
u32 phy_update_test_tick = 0;
u32 phy_update_test_seq = 0;

void feature_2m_phy_conn_mainloop(void)
{
	if(device_connection_tick && clock_time_exceed(device_connection_tick, 2000000)){
		device_connection_tick = 0;
		phy_update_test_tick = clock_time() | 1;
		phy_update_test_seq = 0;  //reset
	}

	if(phy_update_test_tick && clock_time_exceed(phy_update_test_tick, 7000000)){
		phy_update_test_tick = clock_time() | 1;

		if((phy_update_test_seq++)%2 == 0){
			blc_ll_setPhy(BLS_CONN_HANDLE, PHY_TRX_PREFER, PHY_PREFER_2M, PHY_PREFER_2M);
		}
		else{
			blc_ll_setPhy(BLS_CONN_HANDLE, PHY_TRX_PREFER, PHY_PREFER_1M, PHY_PREFER_1M);
		}

		//phy_update_test_tick = 0;
		//blc_ll_setPhy(BLS_CONN_HANDLE, PHY_TRX_PREFER, PHY_PREFER_2M, PHY_PREFER_2M);
	}

	#if(BLE_PM_ENABLE)
		bls_pm_setSuspendMask(SUSPEND_ADV | SUSPEND_CONN);
	#endif
}

#endif // end of (FEATURE_TEST_MODE == TEST_2M_PHY_CONNECTION)

