/********************************************************************************************************
 * @file     printf.h 
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
 * @date     Sep. 30, 2010
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



#if(SIMULATE_UART_EN)
	int my_printf(const char *fmt, ...);
	int my_sprintf(char* s, const char *fmt, ...);
	void array_printf(unsigned char*data, unsigned int len);
	#define printf	     my_printf
	#define sprintf	     my_sprintf
    #define print_array  array_printf
#else
	#define printf
	#define sprintf
	#define print_array
#endif




