/********************************************************************************************************
 * @file     app.c
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
#include "app.h"
#include <stack/ble/ble.h>
#include "tl_common.h"
#include "drivers.h"
#include "app_config.h"
#include "vendor/common/blt_led.h"
#include "vendor/common/keyboard.h"
#include "vendor/common/blt_soft_timer.h"


void user_init(void)
{
	#if(   FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY \
		|| FEATURE_TEST_MODE == TEST_ADV_IN_CONN_SLAVE_ROLE \
		|| FEATURE_TEST_MODE == TEST_SCAN_IN_ADV_AND_CONN_SLAVE_ROLE \
		|| FEATURE_TEST_MODE == TEST_ADV_SCAN_IN_CONN_SLAVE_ROLE)

		feature_linklayer_state_test_init();

	#elif(FEATURE_TEST_MODE == TEST_POWER_ADV)

		feature_adv_power_test_init();

	#elif(FEATURE_TEST_MODE == TEST_SMP_SECURITY)

		feature_security_test_init_normal();

	#elif(FEATURE_TEST_MODE == TEST_GATT_SECURITY)

		feature_gatt_security_test_init_normal();

	#elif(FEATURE_TEST_MODE == TEST_DATA_LENGTH_EXTENSION)

		feature_sdle_test_init();

	#elif(FEATURE_TEST_MODE == TEST_USER_BLT_SOFT_TIMER)

		feature_soft_timer_test_init();

	#elif(FEATURE_TEST_MODE == TEST_WHITELIST)

		feature_whitelist_test_init();

	#elif(FEATURE_TEST_MODE == TEST_BLE_PHY)

		feature_phytest_init();

	#elif(FEATURE_TEST_MODE == TEST_2M_PHY_CONNECTION)

		feature_2m_phy_conn_init();

	#endif
}


u32 tick_loop=0;
/*----------------------------------------------------------------------------*/
/*-------- Main Loop                                                ----------*/
/*----------------------------------------------------------------------------*/
void main_loop (void)
{
	tick_loop++;

	#if(FEATURE_TEST_MODE == TEST_USER_BLT_SOFT_TIMER)
		blt_soft_timer_process(MAINLOOP_ENTRY);
		feature_soft_timer_test_mainloop();
	#endif

	blt_sdk_main_loop();

	#if(FEATURE_TEST_MODE == TEST_SMP_SECURITY)
		feature_security_test_mainloop();

	#elif(FEATURE_TEST_MODE == TEST_DATA_LENGTH_EXTENSION)
		feature_sdle_test_mainloop();

	#elif(FEATURE_TEST_MODE == TEST_2M_PHY_CONNECTION)
		feature_2m_phy_conn_mainloop();

	#elif(FEATURE_TEST_MODE == TEST_ADV_IN_CONN_SLAVE_ROLE)
		feature_linklayer_state_test_main_loop();

	#elif(FEATURE_TEST_MODE == TEST_POWER_ADV)
		feature_adv_power_test_mainloop();

	#elif(FEATURE_TEST_MODE == TEST_WHITELIST)
		feature_whitelist_test_mainloop();

	#endif
}

/*----------------------------- End of File ----------------------------------*/


