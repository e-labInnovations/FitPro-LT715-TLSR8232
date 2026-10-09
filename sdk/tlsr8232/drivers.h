/********************************************************************************************************
 * @file     drivers.h
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

#include "config.h"

#if (__TL_LIB_5316__ || (MCU_CORE_TYPE == MCU_CORE_5316))
#include "drivers/5316/driver_5316.h"
#elif(__TL_LIB_5317__ || (MCU_CORE_TYPE == MCU_CORE_5317))
#include "drivers/5317/driver_5317.h"
#endif




