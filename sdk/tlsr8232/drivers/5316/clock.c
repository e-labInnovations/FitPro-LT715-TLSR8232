/********************************************************************************************************
 * @file     clock.c
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
 *
 *******************************************************************************************************/

#include "register.h"
#include "compiler.h"
#include "clock.h"
#include "irq.h"
#include "analog.h"

int SYS_TICK_DIV = 0;

/**
 * @brief       This function to select the system clock source.
 * @param[in]   SYS_CLK - the clock source of the system clock.
 * @return      none
 */
void clock_init(SYS_CLK_TYPEDEF SYS_CLK)
{
	if(SYS_CLK == SYS_CLK_16M_Crystal){
		SYS_TICK_DIV = 1;
	}
	else if(SYS_CLK == SYS_CLK_32M_Crystal){
		SYS_TICK_DIV = 2;
	}
	else if(SYS_CLK == SYS_CLK_48M_Crystal){
		SYS_TICK_DIV = 3;
	}
	else{
		//debug. Do not support this system clock rate.
	}

	reg_clk_sel = (unsigned char)SYS_CLK;

	/* WatchDog Configuration */
	#if(MODULE_WATCHDOG_ENABLE)
		wd_startEx(WATCHDOG_INIT_TIMEOUT);
	#endif

	/* Timer0 initialization for BLE */
	reg_tmr_ctrl8 &= (~FLD_TMR0_EN);
	reg_tmr0_tick = (clock_time()*SYS_TICK_DIV);
	reg_irq_mask |=	FLD_IRQ_TMR0_EN;  //Enable Timer0 Interrupt
	write_reg8(0x63c,0x01); //continuous tick mode
	reg_tmr_ctrl8 |= FLD_TMR0_EN;  //Enable Timer0
}

/**
 * @brief       This function is to accelerate the oscillation process by using PWM
 * @param[in]   none
 * @return      none
 */
void pwm_kick_32k_pad(void)
{
	unsigned char reg_66 = READ_REG8(0x66);
	WRITE_REG8(0x66,0x43);  ///select 16M system clock.

	//1.set pb6, pb7 as pwm output
	unsigned char reg_58e = READ_REG8(0x58e);
	WRITE_REG8(0x58e,reg_58e&0x3f);
	unsigned char reg_5ab = READ_REG8(0x5ab);
	WRITE_REG8(0x5ab,reg_5ab&0x0f);
	WRITE_REG8(0x781,0xf3);//pwm clk div

	unsigned short reg_794 = READ_REG16(0x794);
	WRITE_REG16(0x794,0x01);//pwm0's high time or low time
	unsigned short reg_796 = READ_REG16(0x796);
	WRITE_REG16(0x796,0x02);//pwm0's cycle time
	WRITE_REG8(0x780,0x01);//enable pwm0
	WaitMs(5); //25
	unsigned short reg_798 = READ_REG16(0x798);
	WRITE_REG16(0x798,0x01);//pwm1's high time or low time
	unsigned short reg_79a = READ_REG16(0x79a);
	WRITE_REG16(0x79a,0x02);//pwm1's cycle time
	WRITE_REG8(0x780,0x03);//enable pwm1

	//2.wait for pwm wake up xtal
	WaitMs(5); //25
	//3.recover pb6, pb7 as xtal pin
	WRITE_REG8(0x780,0x02);
	WRITE_REG16(0x794,reg_794);
	WRITE_REG16(0x796,reg_796);
	WRITE_REG8(0x780,0x00);
	WRITE_REG16(0x798,reg_798);
	WRITE_REG16(0x79a,reg_79a);

	WRITE_REG8(0x781,0x00);
	WRITE_REG8(0x66,reg_66);
	WRITE_REG8(0x58a,READ_REG8(0x58a)|0xc0);
	WRITE_REG8(0x58e,reg_58e|0xc0);

}

unsigned int clock_time_exceed(unsigned int ref, unsigned int span_us)
{
 	return ((unsigned int)(clock_time() - ref) > span_us * sys_tick_per_us);
}


_attribute_ram_code_ void sleep_us (unsigned int us)
{
	unsigned int t = clock_time();
	while(!clock_time_exceed(t, us)){}
}


/**
 * @Brief:  24M RC Calibration.(error: 0.01%)
 * @Param:  None.
 * @Return: None.
 */
void MCU_24M_RC_ClockCalibrate(void)
{
	unsigned char temp = 0;

	/* Reset to default value */
	analog_write(0x83,0x34);

	/* cap from analog register */
	temp = analog_read(0x02);
	temp |= (1<<4);
	analog_write(0x02,temp);

	/*Disable 24M RC calibration.*/
	temp = analog_read(0x83);
	temp &= ~(1<<0);
	temp &= ~(1<<1);
	analog_write(0x83,temp);

	for(volatile int i=0; i<100; i++);

	/* Enable 24M RC calibration. */
	temp = analog_read(0x83);
	temp |= (1<<0);
	analog_write(0x83,temp);

	/* Wait Calibration completely. */
	for(volatile int i=0; i<10000; i++)
	{
	   if((analog_read(0x84) & 0x01))
	   {
			unsigned char CalValue = 0;
			CalValue = analog_read(0x85);
			analog_write(0x30,CalValue);

			break;
	   }
	}

	/* Disable 24M RC calibration. */
	temp = analog_read(0x83);
	temp &= ~(1<<0);
	analog_write(0x83,temp);

	/* cap from pm_top */
	temp = analog_read(0x02);
	temp &= ~(1<<4);
	analog_write(0x02,temp);
}

/*----------------------------- End of File ----------------------------------*/
