/********************************************************************************************************
 * @file     app_timer.c
 *
 * @brief    This is the source file for TLSR8232
 *
 * @author	 BLE Group
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

#if (DRIVER_TEST_MODE == TEST_HW_TIMER)

#define TIMER_SYS_CLOCK_MODE 	1
#define TIMER_GPIO_TRIGGER_MODE 2
#define TIMER_GPIO_WIDTH_MODE 	3
#define TIMER_TICK_MODE 		4

#define TIMER_MODE				TIMER_TICK_MODE
//////////////////////////////////////
#define GPIO_TRIGGER_PIN        GPIO_PC2
#define GPIO_PULSE_PIN          GPIO_PC2

void app_timerModule_led_init(void){
	//1.init the LED pin,for indication
	gpio_set_func(GPIO_LED ,AS_GPIO);
	gpio_set_output_en(GPIO_LED, 1); 		//enable output
	gpio_set_input_en(GPIO_LED ,0);			//disable input
	gpio_write(GPIO_LED, 0);              	//LED On
}

void app_timer_test_init(void){
	WaitMs(2000);  //leave enough time for SWS_reset when power on

	#if (TIMER_MODE==TIMER_SYS_CLOCK_MODE)
		timer2_set_mode(TIMER_MODE_SYSCLK,0,1000 * CLOCK_SYS_CLOCK_1MS);
		timer_start(TIMER2);
	#elif(TIMER_MODE==TIMER_GPIO_TRIGGER_MODE)
		timer2_gpio_init(GPIO_TRIGGER_PIN, GPIO_Pol_falling);
		irq_enable();
		timer2_set_mode(TIMER_MODE_GPIO_TRIGGER,0,3);
		timer_start(TIMER2);
	#elif(TIMER_MODE==TIMER_GPIO_WIDTH_MODE)
		timer2_gpio_init(GPIO_PULSE_PIN, GPIO_Pol_falling);
		irq_enable();
		timer2_set_mode(TIMER_MODE_GPIO_WIDTH,0,0);
		timer_start(TIMER2);
	#elif(TIMER_MODE==TIMER_TICK_MODE)
		timer2_set_mode(TIMER_MODE_TICK,0,0);
		timer_start(TIMER2);
	#endif
}

int timer2_irq_cnt = 0;
unsigned int gpio_width =0;

_attribute_ram_code_ void app_timer_test_irq_proc(void){
	#if(TIMER_MODE == TIMER_GPIO_TRIGGER_MODE)

		if(reg_tmr_sta & FLD_TMR_STA_TMR2)
		{
			reg_tmr_sta |= FLD_TMR_STA_TMR2; //clear irq status

			timer2_irq_cnt ++;
			gpio_toggle(GPIO_LED);
		}
	#elif(TIMER_MODE == TIMER_GPIO_WIDTH_MODE)

		if(reg_tmr_sta & FLD_TMR_STA_TMR2)
		{
			reg_tmr_sta |= FLD_TMR_STA_TMR2; //clear irq status

			gpio_width = reg_tmr2_tick;
			reg_tmr2_tick = 0;
		}
	#endif
}


#endif /////end of #if (DRIVER_TEST_MODE == TEST_HW_TIMER)
