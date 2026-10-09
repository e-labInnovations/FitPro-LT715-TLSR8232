/********************************************************************************************************
 * @file     app_emi.c
 *
 * @brief    This is  the ADC of application for TLSR8232
 *
 * @author   junyuan.zhang ; junwei.lu
 * @date     August. 30, 2019
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
#include "tl_common.h"
#include "drivers.h"

struct  test_list_s {
	unsigned char  cmd_id;
	void	 (*func)(RF_ModeTypeDef rf_mode,RF_TxPowerTypeDef pwr,signed char rf_chn);
};

#define     RF_RSSI					0x8004
#define 	PACKTET_NUMBER_MODE		0X8005
#define		RUM_CMD					0x8006
#define		RIGIG_CMD				0X8007
#define 	RF_POWER_LEVEL			0X8008
#define 	RF_CHN					0x8009
#define 	RF_MODE					0x800a
#define 	RF_HOP_ENABLE			0x800b
#define     RF_REC_NUMB				0x800c


#define CAP_VALUE					0x77000
#define TP_INOF_ADDR				0x77040


#define 	LED1 					GPIO_PC1

unsigned char  mode=1;//1
unsigned char  power_level = 0;
unsigned char  chn = 2;//2
unsigned char  cmd_now=1;//2
unsigned char  run=1;

void read_flash_para(void)
{
	unsigned char cap;
	flash_read_page(CAP_VALUE,1,&cap);
	if(cap != 0xff && cap > 0xbf && cap < 0xe0 )
	{
		WriteAnalogReg(0x81,cap);
	}
	else
	{
		WriteAnalogReg(0x81,0xd0);
	}
	if( ((*(unsigned char*) (TP_INOF_ADDR)) != 0xff) && ((*(unsigned char*) (TP_INOF_ADDR+1)) != 0xff) ){
	 rf_update_tp_value(*(unsigned char*) (TP_INOF_ADDR), *(unsigned char*) (TP_INOF_ADDR+1));
	}

	if ( ((*(unsigned char*) (TP_INOF_ADDR+2)) != 0xff) && ((*(unsigned char*) (TP_INOF_ADDR+3)) != 0xff) ){
	 rf_load_2m_tp_value(*(unsigned char*) (TP_INOF_ADDR+2), *(unsigned char*) (TP_INOF_ADDR+3));
	}
}

void emi_init(void)
{
//	write_reg32(0x408,	0x29417671 );//access code  0xf8118ac9

	write_reg8(RUM_CMD,	run);//run
	write_reg8(RIGIG_CMD,	cmd_now);//cmd
	write_reg8(RF_POWER_LEVEL,	power_level);//power
	write_reg8(RF_CHN,	chn);//chn
	write_reg8(RF_MODE,	mode);//mode
	write_reg8(RF_RSSI,	0);
	write_reg32(RF_REC_NUMB,	0);
}

void emicarrieronly(RF_ModeTypeDef rf_mode,RF_TxPowerTypeDef pwr,signed char rf_chn)
{
	rf_emi_single_tone(pwr,rf_chn);
	while( ((read_reg8(RUM_CMD)) == run ) &&  ((read_reg8(RIGIG_CMD)) == cmd_now )\
			&& ((read_reg8(RF_POWER_LEVEL)) == power_level ) &&  ((read_reg8(RF_CHN)) == chn )\
			&& ((read_reg8(RF_MODE)) == mode ))
	{
	}
	rf_set_tx_rx_off();
}

void emi_con_prbs9(RF_ModeTypeDef rf_mode,RF_TxPowerTypeDef pwr,signed char rf_chn)
{
	rf_emi_tx_continue_setup(rf_mode,pwr,rf_chn,0);

	while( ((read_reg8(RUM_CMD)) == run ) &&  ((read_reg8(RIGIG_CMD)) == cmd_now )\
			&& ((read_reg8(RF_POWER_LEVEL)) == power_level ) &&  ((read_reg8(RF_CHN)) == chn )\
			&& ((read_reg8(RF_MODE)) == mode ))
	{
		rf_continue_mode_run();
	}
	rf_set_tx_rx_off();
}

void emi_con_tx0f(RF_ModeTypeDef rf_mode,RF_TxPowerTypeDef pwr,signed char rf_chn)
{
	rf_emi_tx_continue_setup(rf_mode,pwr,rf_chn,1);

	while( ((read_reg8(RUM_CMD)) == run ) &&  ((read_reg8(RIGIG_CMD)) == cmd_now )\
			&& ((read_reg8(RF_POWER_LEVEL)) == power_level ) &&  ((read_reg8(RF_CHN)) == chn )\
			&& ((read_reg8(RF_MODE)) == mode ))
	{
		rf_continue_mode_run();
	}
	rf_set_tx_rx_off();
}

void emi_con_tx55(RF_ModeTypeDef rf_mode,RF_TxPowerTypeDef pwr,signed char rf_chn)
{
	rf_emi_tx_continue_setup(rf_mode,pwr,rf_chn,2);

	while( ((read_reg8(RUM_CMD)) == run ) &&  ((read_reg8(RIGIG_CMD)) == cmd_now )\
			&& ((read_reg8(RF_POWER_LEVEL)) == power_level ) &&  ((read_reg8(RF_CHN)) == chn )\
			&& ((read_reg8(RF_MODE)) == mode ))
	{
		rf_continue_mode_run();
	}
	rf_set_tx_rx_off();
}

void emitxprbs9(RF_ModeTypeDef rf_mode,RF_TxPowerTypeDef pwr,signed char rf_chn)
{
	rf_emi_tx_brust_setup(rf_mode,pwr,rf_chn,0);
	while( ((read_reg8(RUM_CMD)) == run ) &&  ((read_reg8(RIGIG_CMD)) == cmd_now )\
			&& ((read_reg8(RF_POWER_LEVEL)) == power_level ) &&  ((read_reg8(RF_CHN)) == chn )\
			&& ((read_reg8(RF_MODE)) == mode ))
	{
		rf_emi_tx_brust_loop(rf_mode,0);
	}
	rf_set_tx_rx_off();
}

void emitx0f(RF_ModeTypeDef rf_mode,RF_TxPowerTypeDef pwr,signed char rf_chn)
{
	rf_emi_tx_brust_setup(rf_mode,pwr,rf_chn,1);
	while( ((read_reg8(RUM_CMD)) == run ) &&  ((read_reg8(RIGIG_CMD)) == cmd_now )\
			&& ((read_reg8(RF_POWER_LEVEL)) == power_level ) &&  ((read_reg8(RF_CHN)) == chn )\
			&& ((read_reg8(RF_MODE)) == mode ))
	{
		rf_emi_tx_brust_loop(rf_mode,1);
	}
	rf_set_tx_rx_off();
}

void emitx55(RF_ModeTypeDef rf_mode,RF_TxPowerTypeDef pwr,signed char rf_chn)
{
	rf_emi_tx_brust_setup(rf_mode,pwr,rf_chn,2);
	while( ((read_reg8(RUM_CMD)) == run ) &&  ((read_reg8(RIGIG_CMD)) == cmd_now )\
			&& ((read_reg8(RF_POWER_LEVEL)) == power_level ) &&  ((read_reg8(RF_CHN)) == chn )\
			&& ((read_reg8(RF_MODE)) == mode ))
	{
		rf_emi_tx_brust_loop(rf_mode,2);
	}
	rf_set_tx_rx_off();
}

void emirx(RF_ModeTypeDef rf_mode, RF_TxPowerTypeDef pwr,signed char rf_chn)
{
	rf_emi_rx(rf_mode,rf_chn);
	while( ((read_reg8(RUM_CMD)) == run ) &&  ((read_reg8(RIGIG_CMD)) == cmd_now )\
			&& ((read_reg8(RF_POWER_LEVEL)) == power_level ) &&  ((read_reg8(RF_CHN)) == chn )\
			&& ((read_reg8(RF_MODE)) == mode ))
	{
		rf_emi_rx_loop();
		if(rf_emi_get_rxpkt_cnt()!=read_reg32(0x84000c))
		{
			write_reg8(RF_RSSI,rf_emi_get_rssi_avg());
			write_reg32(RF_REC_NUMB,rf_emi_get_rxpkt_cnt());
		}
	}
	rf_set_tx_rx_off();
}

struct  test_list_s  ate_list[] = {
	{0x01,emicarrieronly},
	{0x02,emi_con_prbs9},
	{0x03,emirx},
	{0x04,emitxprbs9},
	{0x05,emitx55},
	{0x06,emitx0f},
	//{0x07,emi_con_tx55},
	//{0x08,emi_con_tx0f},
};

void app_rf_emi_test_start(void)
{
	unsigned char i=0;

	while(1)
	{
	   run = read_reg8(RUM_CMD);  // get the run state!
	   if(run!=0)
	   {

		   IRQ_Disable();
		   power_level = read_reg8(RF_POWER_LEVEL);
		   chn = read_reg8(RF_CHN);
		   mode = read_reg8(RF_MODE);
		   cmd_now = read_reg8(RIGIG_CMD);  // get the command!

			for (i=0; i<sizeof (ate_list)/sizeof (struct test_list_s); i++)
			{
				if(cmd_now == ate_list[i].cmd_id)
				{
					if(mode==0)//ble 2M mode
					{
						rf_drv_init(RF_MODE_BLE_2M);
						ate_list[i].func(RF_MODE_BLE_2M, power_level, chn);
					}
					else if(mode==1)//ble 1M mode
					{
						rf_drv_init(RF_MODE_BLE_1M);
						ate_list[i].func(RF_MODE_BLE_1M,power_level,chn);
					}
					break;
				}
			}
			run = 0;
			write_reg8(RUM_CMD, run);
	   }
	}

}

void app_emi_init()
{
	read_flash_para();
	emi_init();
	rf_set_power_level_index (power_level);
	rf_set_tx_rx_off_auto_mode();
}

