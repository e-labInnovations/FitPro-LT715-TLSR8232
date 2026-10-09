/********************************************************************************************************
 * @file     main.c
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
#include <tl_common.h>
#include "drivers.h"
#include "stack/ble/ble.h"
#include "../common/user_config.h"

#if (REMOTE_IR_ENABLE)
#include "rc_ir.h"
#endif

extern void user_init();
extern void main_loop (void);
extern _attribute_ram_code_ void app_pwm_ir_test_proc(void);
extern _attribute_ram_code_ void app_uart_test_irq_proc(void);
extern _attribute_ram_code_ void app_i2c_test_irq_proc(void);
extern _attribute_ram_code_ void app_spi_test_irq_proc(void);
extern _attribute_ram_code_ void app_gpio_test_irq_proc(void);

_attribute_ram_code_ void irq_handler(void)
{
	#if (DRIVER_TEST_MODE == TEST_HW_TIMER)
		app_timer_test_irq_proc();
	#elif (DRIVER_TEST_MODE == TEST_PWM)
		app_pwm_irq_test_proc();
	#elif(DRIVER_TEST_MODE == TEST_UART)
		app_uart_test_irq_proc();
	#elif(DRIVER_TEST_MODE == TEST_IIC)
		app_i2c_test_irq_proc();
	#elif(DRIVER_TEST_MODE == TEST_SPI)
		app_spi_test_irq_proc();
	#elif(DRIVER_TEST_MODE == TEST_GPIO_IRQ)
		app_gpio_test_irq_proc();
	#endif
}

int main(void){

	blc_pm_select_internal_32k_crystal();

	cpu_wakeup_init();

	clock_init(SYS_CLK_16M_Crystal);

	gpio_init();

	/* load customized freq_offset CAP value and TP value.*/
	blc_app_loadCustomizedParameters(); //do this operation before "rf_drv_init"

	rf_drv_init(RF_MODE_BLE_1M);

	user_init ();

	while (1) {
	#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
	#endif
		main_loop ();
	}
}
