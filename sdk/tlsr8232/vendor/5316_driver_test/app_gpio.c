/********************************************************************************************************
 * @file     app_gpio.c
 *
 * @brief    This is the source file for TLSR8232
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
 * @par      History:
 * 			 1.initial release(DEC. 26 2018)
 *
 * @version  A001
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"

#if (DRIVER_TEST_MODE == TEST_GPIO_IRQ)

#define GPIO_IRQ				1
#define GPIO_IRQ_RSIC0			2
#define GPIO_IRQ_RSIC1			3
#define GPIO_IRQ_RSIC2			4
#define GPIO_HIGH_RESISTOR		5

#define GPIO_MODE 				GPIO_HIGH_RESISTOR
//////////////////
#define GPIO_TEST_PIN1          GPIO_PC1
#define GPIO_TEST_PIN2          GPIO_PC2
#define GPIO_TEST_PIN3          GPIO_PC3
#define GPIO_TEST_PIN4          GPIO_PC4
//////

////init led
void app_gpioModule_led_init(void){
	gpio_set_func(GPIO_LED ,AS_GPIO);
	gpio_set_output_en(GPIO_LED, 1); 		//enable output
	gpio_set_input_en(GPIO_LED ,0);			//disable input
	gpio_write(GPIO_LED, 0);              	//LED On
}

void app_gpio_test_init(void){

	WaitMs(2000);
	//2.init the SW1 pin,for trigger interrupt
	#if (GPIO_MODE == GPIO_IRQ )
		gpio_set_func(GPIO_TEST_PIN1 ,AS_GPIO);
		gpio_set_output_en(GPIO_TEST_PIN1, 0); 			//enable output
		gpio_set_input_en(GPIO_TEST_PIN1 ,1);				//disable input
		gpio_setup_up_down_resistor(GPIO_TEST_PIN1, PM_PIN_PULLUP_10K);
		gpio_set_interrupt(GPIO_TEST_PIN1, GPIO_Pol_falling);

	#elif(GPIO_MODE == GPIO_IRQ_RSIC0)
		gpio_set_func(GPIO_TEST_PIN2 ,AS_GPIO);
		gpio_set_output_en(GPIO_TEST_PIN2, 0); 			//enable output
		gpio_set_input_en(GPIO_TEST_PIN2 ,1);				//disable input
		gpio_setup_up_down_resistor(GPIO_TEST_PIN2, PM_PIN_PULLUP_10K);
		gpio_set_interrupt_risc0(GPIO_TEST_PIN2, GPIO_Pol_falling);

	#elif(GPIO_MODE == GPIO_IRQ_RSIC1)
		gpio_set_func(GPIO_TEST_PIN3 ,AS_GPIO);
		gpio_set_output_en(GPIO_TEST_PIN3, 0); 			//enable output
		gpio_set_input_en(GPIO_TEST_PIN3 ,1);				//disable input
		gpio_setup_up_down_resistor(GPIO_TEST_PIN3, PM_PIN_PULLUP_10K);
		gpio_set_interrupt_risc1(GPIO_TEST_PIN3, GPIO_Pol_falling);

	#elif(GPIO_MODE == GPIO_IRQ_RSIC2)
		gpio_set_func(GPIO_TEST_PIN4 ,AS_GPIO);
		gpio_set_output_en(GPIO_TEST_PIN4, 0);
		gpio_set_input_en(GPIO_TEST_PIN4 ,1);				//disable input
		gpio_setup_up_down_resistor(GPIO_TEST_PIN4, PM_PIN_PULLUP_10K);
		gpio_set_interrupt_risc2(GPIO_TEST_PIN4, GPIO_Pol_falling);

	#elif(GPIO_MODE == GPIO_HIGH_RESISTOR)
		gpio_shutdown(GPIO_ALL);				//set all gpio as high resistor except sws and mspi

	#endif

	irq_enable();
}

int gpio_irq_cnt = 0;

_attribute_ram_code_ void app_gpio_test_irq_proc(void){
	#if (GPIO_MODE == GPIO_IRQ )

		if((reg_irq_src & FLD_IRQ_GPIO_EN)==FLD_IRQ_GPIO_EN){
			reg_irq_src |= FLD_IRQ_GPIO_EN; // clear the relevant irq
			if(gpio_read(GPIO_TEST_PIN1)== 0){ // press key with low level to flash light
				WaitMs(10);
				if(gpio_read(GPIO_TEST_PIN1)== 0){
					gpio_irq_cnt++;
					DBG_CHN0_TOGGLE;
					gpio_toggle(GPIO_LED);
				}
			}
		}

	#elif(GPIO_MODE == GPIO_IRQ_RSIC0)

		if((reg_irq_src & FLD_IRQ_GPIO_RISC0_EN)==FLD_IRQ_GPIO_RISC0_EN){
			reg_irq_src |= FLD_IRQ_GPIO_RISC0_EN; // clear the relevant irq

			if(gpio_read(GPIO_TEST_PIN2)== 0){ // press key with low level to flash light
				WaitMs(10);
				if(gpio_read(GPIO_TEST_PIN2)== 0){
					gpio_irq_cnt++;
					DBG_CHN0_TOGGLE;
					gpio_toggle(GPIO_LED);
				}
			}
		}

	#elif(GPIO_MODE == GPIO_IRQ_RSIC1)

		if((reg_irq_src & FLD_IRQ_GPIO_RISC1_EN)==FLD_IRQ_GPIO_RISC1_EN){
			reg_irq_src |= FLD_IRQ_GPIO_RISC1_EN; // clear the relevant irq

			if(gpio_read(GPIO_TEST_PIN3)== 0){ // press key with low level to flash light
				WaitMs(10);
				if(gpio_read(GPIO_TEST_PIN3)== 0){
					gpio_irq_cnt++;
					DBG_CHN0_TOGGLE;
					gpio_toggle(GPIO_LED);
				}
			}
		}

	#elif(GPIO_MODE == GPIO_IRQ_RSIC2)

		if((reg_irq_src & FLD_IRQ_GPIO_RISC2_EN)==FLD_IRQ_GPIO_RISC2_EN){
			reg_irq_src |= FLD_IRQ_GPIO_RISC2_EN; // clear the relevant irq

			if(gpio_read(GPIO_TEST_PIN4)== 0){ // press key with low level to flash light
				WaitMs(10);
				if(gpio_read(GPIO_TEST_PIN4)== 0){
					gpio_irq_cnt++;
					DBG_CHN0_TOGGLE;
					gpio_toggle(GPIO_LED);
				}
			}
		}
	#endif
}


#endif  ///end of #if (DRIVER_TEST_MODE == TEST_GPIO_IRQ)

