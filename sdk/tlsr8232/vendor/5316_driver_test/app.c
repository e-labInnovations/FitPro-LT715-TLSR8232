/********************************************************************************************************
 * @file     app.c
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
#include "tl_common.h"
#include "drivers.h"


#if (__PROJECT_5316_DRIVER_TEST__ )

extern void app_timer_test_init(void);
extern void app_timerModule_led_init(void);
extern void app_emi_init(void);
extern void app_gpio_test_init(void);
extern void app_gpioModule_led_init(void);

extern void app_uart_test_init(void);
extern void app_i2c_test_init(void);
extern void app_spi_test_init(void);
extern void app_adc_test_init(void);
extern void app_pwm_test(void);
extern void app_rf_emi_test_start(void);

void user_init()
{
	#if (DRIVER_TEST_MODE == TEST_HW_TIMER)

		app_timer_test_init();
		app_timerModule_led_init();

	#elif(DRIVER_TEST_MODE == TEST_GPIO_IRQ)

		app_gpio_test_init();
		app_gpioModule_led_init();

	#elif(DRIVER_TEST_MODE == TEST_UART)

		app_uart_test_init();

	#elif(DRIVER_TEST_MODE == TEST_IIC)

		app_i2c_test_init();

	#elif(DRIVER_TEST_MODE == TEST_SPI)

		app_spi_test_init();

	#elif(DRIVER_TEST_MODE == TEST_ADC)

		app_adc_test_init();

	#elif(DRIVER_TEST_MODE == TEST_PWM)
		app_pwm_test();
	#elif(DRIVER_TEST_MODE == TEST_LOW_POWER)
		///v1.1.0 not support
	#elif(DRIVER_TEST_MODE == TEST_RF_EMI)
		app_emi_init();
	#else

	#endif

}

/*----------------------------------------------------------------------------*/
/*--------- Main Loop                                             ------------*/
/*----------------------------------------------------------------------------*/
void main_loop (void)
{

	#if (DRIVER_TEST_MODE  == TEST_UART)
		app_uart_test_start();
	#elif(DRIVER_TEST_MODE == TEST_IIC)
		app_i2c_test_start();
	#elif(DRIVER_TEST_MODE == TEST_SPI)
		app_spi_test_start();
	#elif(DRIVER_TEST_MODE == TEST_ADC)
		app_adc_test_start();
	#elif(DRIVER_TEST_MODE == TEST_RF_EMI)
		app_rf_emi_test_start();
	#endif

}



#endif  //end of __PROJECT_5316_DRIVER_TEST__

