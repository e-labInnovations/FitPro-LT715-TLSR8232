/********************************************************************************************************
 * @file     random.h
 *
 * @brief    This is the header file for TLSR8232
 *
 * @author	 junyuna.zhang
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

#include "compiler.h"

/**
 * @brief     This function performs to preparatory initials random generator.
 * @param[in] none.
 * @return    none.
 */

void random_generator_init(void);

/**
 * @brief     This function performs to initialize the rand time in flash/sram.
 *            (example: system clock:16M, code in flash 23us, code in sram 4us)
 * @param[in] none.
 * @return    the value of time.
 */
_attribute_ram_code_ unsigned int rand(void);

/**
 * @brief       generate random number
 * @param[in]   len  - length of buffer
 * @param[out]  data - buffer
 * @return      none
 */
void generateRandomNum(int len, unsigned char *data);
