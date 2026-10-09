/********************************************************************************************************
 * @file     battery_check.c
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
#include "battery_check.h"
#include "tl_common.h"
#include "drivers.h"

int adc_hw_initialized = 0;

/**
 * @Brief:  Bubble sort.
 * @Param:  pData -> pointer point to data
 * @Param:  len -> lenght of data
 * @Return: None.
 */
void bubble_sort(unsigned short *pData, unsigned int len)
{
	for(volatile int i = 0; i< len-1; i++)
	{
		for(volatile int j = 0; j<len-1 - i; j++)
		{
			if(pData[j] > pData[j+1])
			{
				unsigned short temp = pData[j];
				pData[j] = pData[j+1];
				pData[j+1] = temp;
			}
		}
	}
}

int app_suspend_enter_low_battery (void)
{
	if (gpio_read(GPIO_WAKEUP_MODULE))
	{
		return 0;
	}
	return 1;
}

/**
 * @Brief:  Battery check.
 * @Param:  None.
 * @Return: None.
 */
volatile signed short adcValue[BATTERY_SAMPLE_NUM];
void battery_power_check(int minVol_mV)
{

	if(!adc_hw_initialized){
		adc_hw_initialized = 1;
		adc_init();
		adc_vbat_init(BATTERY_CHECK_PIN);
	}

	adc_reset();
	aif_reset(); ///refer to driver
	adc_power_on(1);

	//clear adcValue buffer
	for(volatile int i=0; i<BATTERY_SAMPLE_NUM; i++){
		adcValue[i] = 0;
	}

	sleep_us(25);//must wait 2 adc cycle;
	//dfifo setting will lose in suspend/deep, so we need config it every time
	adc_aif_set_misc_buf((unsigned short *)adcValue,BATTERY_SAMPLE_NUM);  //size: BATTERY_SAMPLE_NUM*4
	adc_aif_set_m_chn_en(1);
	adc_aif_set_use_raw_data_en();

	/* Get ADC value. */
	unsigned char i;
	unsigned short adc_sample[BATTERY_SAMPLE_NUM] = {0};
	unsigned int t0;
	t0 = clock_time();
	for(i=0;i<BATTERY_SAMPLE_NUM;i++){

		//When the data is not zero or more than 1.5 sampling times (when the data is zero),The default data is already ready.
		while((!adcValue[i])&&(!clock_time_exceed(t0,20)));  //wait for new adc sample data,

		t0 = clock_time();

		if(adcValue[i] & BIT(13)){  //14 bit resolution, BIT(13) is sign bit, 1 means negative voltage in differential_mode
			adc_sample[i] = 0;
		}
		else{
			adc_sample[i] = ((unsigned short)adcValue[i] & 0x1FFF);  //BIT(12..0) is valid adc result
		}
	}
	//Power off ADC and DFIFO2 for saving power
	adc_power_on(0); //	adc_power_on_sar_adc(0);
	adc_aif_set_m_chn_en(0); //reg_dfifo_mode &= ~DFIFO_Mode_FIFO2_Input;

	////just for example. user can use other method.
	bubble_sort(adc_sample, BATTERY_SAMPLE_NUM);
	u32 adcValueAvg = (adc_sample[2] + adc_sample[3] + adc_sample[4] + adc_sample[5]) >> 2;

	//////////////// adc sample data convert to voltage(mv) ////////////////
	//                         (1180mV Vref, 1/8 scaler)   (BIT<12~0> valid data)
	// 			 =  adc_result   *   1160     * 8        /        0x2000
	//           =  adc_result * 4680 >>12
	//           =  adc_result * 295 >>8
	//	u16 vol = (adcValueAvg * 1180 * 8)>>13;//Unit:mV; 由于参考电压不准，实际的参考电压为1.18V(Vref = 1.2V);*8 indicate 1/8 scaler
	u16 vol;
	if((adc_cal_value!=0xffff)&&(adc_cal_value != 0x0000))  //Already calibrated
	{
		vol  = adcValueAvg*1000/adc_cal_value;       //this used 1000mV calibrated value
	}
	else
	{
		vol  = (adcValueAvg * 295)>>8; ////vol = (adcValueAvg * 1180 * 8)>>13;//Unit:mV; 由于参考电压不准，实际的参考电压为1.18V(Vref = 1.2V);*8 indicate 1/8 scaler
	}

	/* Low voltage processing. Enter deep sleep. */
	if(vol < minVol_mV){

		#if (1 && BLT_APP_LED_ENABLE)  //led indicate
			gpio_set_output_en(GPIO_LED, 1);  //output enable
			for(int k=0;k<3;k++){
				gpio_write(GPIO_LED, 1);
				sleep_us(200000);
				gpio_write(GPIO_LED, 0);
				sleep_us(200000);
			}
			gpio_set_output_en(GPIO_LED, 0);
		#endif

		GPIO_WAKEUP_MODULE_LOW;
		bls_pm_registerFuncBeforeSuspend( &app_suspend_enter_low_battery );

		analog_write(DEEP_ANA_REG2, BATTERY_VOL_LOW);
		cpu_sleep_wakeup(PM_SLeepMode_Deep, PM_WAKEUP_PAD, 0);
	}
}
