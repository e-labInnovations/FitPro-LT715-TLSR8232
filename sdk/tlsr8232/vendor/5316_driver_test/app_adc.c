/********************************************************************************************************
 * @file     app_adc.c
 *
 * @brief    This is  the ADC of application for TLSR8232
 *
 * @author   junyuan.zhang ; junwei.lu
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
 * @par      History:
 * 			 1.initial release(DEC. 26 2018)
 *
 * @version  A001
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"

#if (DRIVER_TEST_MODE == TEST_ADC)

	#define ADC_CHECK_PIN     GPIO_PB2 ///GPIO_PB3

	void app_adc_test_init(void){
		adc_init();
		adc_base_init(ADC_CHECK_PIN); ////PA6/PA7; PB0~PB7
	}


	u32 tick_adc_sample = 0;
	u16 current_adc_val = 0;
	void app_adc_test_start(void){
		if (clock_time_exceed(tick_adc_sample, 200*1000)){   /////entry code per 200ms

			tick_adc_sample = clock_time();

			current_adc_val = adc_set_sample_and_get_result();
		}
	}

#endif //////
