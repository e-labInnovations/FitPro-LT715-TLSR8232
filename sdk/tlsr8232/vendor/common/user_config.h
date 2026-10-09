/********************************************************************************************************
 * @file     user_config.h 
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


#if(__PROJECT_5316_BLE_REMOTE__)
	#include "../5316_ble_remote/app_config.h"
#elif(__PROJECT_5316_BLE_SAMPLE__)
	#include "vendor/5316_ble_sample/app_config.h"
#elif(__PROJECT_5316_DUAL_MODE__)
	#include "../5316_dual_mode/app_config.h"
#elif(__PROJECT_5316_HCI__)
	#include "../5316_hci/app_config.h"
#elif(__PROJECT_5316_MODULE__)
	#include "../5316_module/app_config.h"
#elif(__PROJECT_5316_DRIVER_TEST__)
	#include "../5316_driver_test/app_config.h"
#elif(__PROJECT_5316_FEATURE_TEST__)
	#include "../5316_feature_test/app_config.h"
#else
	#include "../common/default_config.h"
#endif

