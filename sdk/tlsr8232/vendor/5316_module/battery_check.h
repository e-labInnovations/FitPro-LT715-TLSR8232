/********************************************************************************************************
 * @file     battery_check.h
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
#ifndef BATTERY_CHECK_H_
#define BATTERY_CHECK_H_

#include "drivers.h"

#define BATTERY_CHECK_PIN     GPIO_PA7 ///GPIO_PB3

#define BATTERY_VOL_OK        0x00
#define BATTERY_VOL_LOW       0x01

#define BATTERY_VOL_MIN      (2000)//Unit: mV
#define BATTERY_SAMPLE_NUM       8 ///please make sure this value is x*8 (integer multiple of eight)

void battery_power_check(int minVol_mV);

#endif /* BATTERY_CHECK_H_ */
