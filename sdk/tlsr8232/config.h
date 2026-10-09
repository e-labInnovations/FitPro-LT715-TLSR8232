/********************************************************************************************************
 * @file     config.h
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

#define CHIP_TYPE_5316      1
#define CHIP_TYPE_5317      2

#ifndef CHIP_TYPE
#define	CHIP_TYPE 			1000
#endif



#define MCU_CORE_5316       1
#define MCU_CORE_5317       2

#if(CHIP_TYPE == CHIP_TYPE_5316)
	#define MCU_CORE_TYPE   MCU_CORE_5316
#elif(CHIP_TYPE == CHIP_TYPE_5317)
	#define MCU_CORE_TYPE   MCU_CORE_5317
#else
	#define MCU_CORE_TYPE	1000
#endif


