/********************************************************************************************************
 * @file     app_pwm.c
 *
 * @brief    This is the source file for TLSR8232
 *
 * @author	 junyuan.zhang ;junwei.lu
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

#if (DRIVER_TEST_MODE == TEST_PWM)

#define	 PWM_NORMAL			1
#define	 PWM_IR_DMA_FIFO	5 ///this mode is easier than IR FIFO.so user can ignore IR FIFO mode

#define	 TEST_PWM_SELECT	PWM_NORMAL





#define IR_CARRIER_FREQ				38000  	// 1 frame -> 1/38k -> 1000/38 = 26 us
#define PWM_CARRIER_CYCLE_TICK		( CLOCK_SYS_CLOCK_HZ/IR_CARRIER_FREQ )  //16M: 421 tick, f = 16000000/421 = 38004,T = 421/16=26.3125 us
#define PWM_CARRIER_HIGH_TICK		( PWM_CARRIER_CYCLE_TICK/3 )   // 1/3 duty


#define PWM_IR_MAX_NUM    64     //user can define this max number
typedef struct{
    unsigned int dma_len;        // dma len
    unsigned short data[PWM_IR_MAX_NUM];
    unsigned int   data_num;
}pwm_dma_data_t;


pwm_dma_data_t T_dmaData_buf;

/*********************************************************************************
	PWM0   :  PA0.  PB3
	PWM0_N :  PB6	PC2

	PWM1   :  PB1. PB7
	PWM1_N :  PA2. PB4

	PWM2   :  PA4. PB2
	PWM2_N :  PC1
 *********************************************************************************/


void app_pwm_test(void){

	WaitMs(1000);

	reg_clk_en1 |= FLD_CLK1_PWM_EN;
	pwm_set_clk(CLOCK_SYS_CLOCK_HZ, CLOCK_SYS_CLOCK_HZ);

	#if (TEST_PWM_SELECT == PWM_NORMAL)

		gpio_set_func(GPIO_PA0, AS_PWM0);
		pwm_set_mode(PWM0_ID, PWM_NORMAL_MODE);
		pwm_set_cycle_and_duty(PWM0_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (500 * CLOCK_SYS_CLOCK_1US) );
		pwm_start(PWM0_ID);

	#elif 1 //(TEST_PWM_SELECT == PWM_IR_DMA_FIFO)

		//only pwm0 support fifo mode
		gpio_set_func(GPIO_PA0, AS_PWM0);
		pwm_set_mode(PWM0_ID, PWM_IR_DMA_FIFO_MODE);
		//pwm_set_phase(PWM0_ID, 0);   //no phase at pwm beginning

		//config TMAX0  & TCMP0: 38k, 1/3 duty
		pwm_set_cycle_and_duty(PWM0_ID, PWM_CARRIER_CYCLE_TICK,  PWM_CARRIER_HIGH_TICK );



		//config waveforms
		T_dmaData_buf.data_num = 0;

	//preamble:  9 ms carrier,  4.5 ms low
		T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(1, PWM0_PULSE_NORMAL, 9000 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);
		T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(0, PWM0_PULSE_NORMAL, 4500 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);

	//data 1 :  560 us carrier,  560 us low
		T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(1, PWM0_PULSE_NORMAL, 560 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);
		T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(0, PWM0_PULSE_NORMAL, 560 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);


	//data  0 :  560 us carrier,  1690 us low
		T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(1, PWM0_PULSE_NORMAL, 560 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);
		T_dmaData_buf.data[T_dmaData_buf.data_num ++]= pwm_config_dma_fifo_waveform(0, PWM0_PULSE_NORMAL, 1690 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);


	//end:  560 us carrier
		T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(1, PWM0_PULSE_NORMAL, 560 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);

		//calculate  dma len
		T_dmaData_buf.dma_len = T_dmaData_buf.data_num * 2;




		pwm_set_dma_address(&T_dmaData_buf);



	//add pwm0 dma fifo done irq, when all waveform send over, this irq will triggers
		//enable mcu global irq
		 irq_enable();

		//enable system irq PWM
		reg_irq_mask |= FLD_IRQ_SW_PWM_EN;

		//enable pwm0 ir dma fifo done irq
		reg_pwm_irq_sta = FLD_IRQ_PWM0_IR_DMA_FIFO_DONE; //clear irq status
		reg_pwm_irq_mask |= FLD_IRQ_PWM0_IR_DMA_FIFO_DONE;


	//PWM0 ir dma fifo mode begin
		pwm_start_dma_ir_sending();

		DBG_CHN0_HIGH;  //debug
	#endif
}

_attribute_ram_code_ void app_pwm_irq_test_proc(void)
{

	if(reg_pwm_irq_sta & FLD_IRQ_PWM0_IR_DMA_FIFO_DONE){
		reg_pwm_irq_sta = FLD_IRQ_PWM0_IR_DMA_FIFO_DONE;
		DBG_CHN0_LOW;  //finish
	}
}

#endif  ////end of (DRIVER_TEST_MODE == TEST_PWM)
