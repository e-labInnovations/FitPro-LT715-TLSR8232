/********************************************************************************************************
 * @file     putchar.h
 *
 * @brief    This is the header file for TLSR8233
 *
 * @author	 junwei.lu
 * @date     May 8, 2018
 *
 * @par      Copyright (c) 2018, Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *           The information contained herein is confidential property of Telink
 *           Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *           of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *           Co., Ltd. and the licensee or the terms described here-in. This heading
 *           MUST NOT be removed from this file.
 *
 *           Licensees are granted free, non-transferable use of the information in this
 *           file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#pragma once

#include "driver_config.h"
#include "common/config/user_config.h"

#ifndef WIN32

#ifndef SIMULATE_UART_EN
	#define SIMULATE_UART_EN          0
#endif

#ifndef DEBUG_TX_PIN
	#define DEBUG_TX_PIN        GPIO_PB4
#endif

#ifndef DEBUG_BAUDRATE
	#define DEBUG_BAUDRATE      (115200)
#endif

int putchar(int c);

#endif


