/*
 * emi.c
 *
 *  Created on: 2019-8-12
 *      Author: Administrator
 */

#include "register.h"
#include "analog.h"
#include "clock.h"
#include "pm.h"
#include "rf_drv.h"
#include "emi.h"

#define STATE0		0x1234
#define STATE1		0x5678
#define STATE2		0xabcd
#define STATE3		0xef01

unsigned char pkt_phytest [64] = {
		39, 0, 0, 0,
		0, 37,
		0, 1, 2, 3, 4, 5, 6, 7
};

static signed  char   rssi=0;
static unsigned int   emi_rx_cnt=0,emi_rssibuf=0;

static unsigned int   state0,state1,state2,state3;
static unsigned char  emi_zigbee_tx_packet[48]  __attribute__ ((aligned (4))) = {19,0,0,0,20,0,0};
static unsigned char  emi_ble_tx_packet [48]  __attribute__ ((aligned (4))) = {39, 0, 0, 0,0, 37};
static unsigned char  emi_rx_packet[64] __attribute__ ((aligned (4)));

void rf_emi_single_tone(RF_TxPowerTypeDef power_level,signed char rf_chn)
{
	//reset manual tx
	WRITE_REG8(0xf02,0x45);
	WaitUs(100);
	WRITE_REG8(0xf02,0x55);

	WRITE_REG16(0x4d6, 2400+rf_chn);
	rf_set_power_level_index (power_level);

	//tx_cyc1_manual on
	WriteAnalogReg(0xa5, 0x04); //tx_cyc1 manual enable
	sub_wr(0x4e8, 1, 2, 2);//manual tx_cyc1 from bb

	WriteAnalogReg(0xa5,0x44);   // for carrier  mode
	WRITE_REG8 (0x4e8, 0x04); // for  carrier mode
}

static unsigned int tpGetCnt(unsigned char precision)
{
	unsigned int tpCnt0, tpCnt1, tpCnt2,tpCnt, done;

	done = 0;
	sub_wr_ana(0x94, precision, 3, 0);
	sub_wr_ana(0x93, 0 , 7, 7);
	sub_wr_ana(0x93, 1 , 7, 7); // set cal_en

	while(done==0)
	{
		tpCnt2 = ReadAnalogReg(0xaf);
    	tpCnt1 = ReadAnalogReg(0xae);
    	tpCnt0 = ReadAnalogReg(0xad);
    	tpCnt  = (tpCnt2<<16)+(tpCnt1<<8) + tpCnt0;

    	done  =  tpCnt & 0x800000;
	}
	tpCnt = tpCnt & 0x7fffff;
	return tpCnt;
}

unsigned char rf_tp_calibration(RF_ModeTypeDef mode, unsigned char chn)
{

	unsigned char pGain;
	unsigned int cnt,vcnt,fdev,fref, ii,iGain,cntExp;
	unsigned char precision;

	precision = 0;
	fref	  = 24;
	vcnt      =  65536;//524288>>(precision+3);
	//fdev      =  fdev/2000; //1000 for 1MHz, 2000 for 2MHz
	pGain	  =  0x40;

	sub_wr_ana(0xab, 0x3, 1, 0);//turn off PA according to wenfeng, TODO test option: [1] [2] pd individually

//	if((mode==RF_MODE_BLE_1M) | (mode==RF_MODE_BLE_1M_NO_PN))
	if(mode==RF_MODE_BLE_1M)
	{
		fdev = 1000;
	}
	else
	{
		fdev = 2000;
	}

	rf_drv_init(mode);
	WRITE_REG16(0x4d6,chn+2400);

	WRITE_REG8(0xf02,0x45);
	WaitUs(100);
	WRITE_REG8(0xf02,0x55);

	//open pll loop
	WriteAnalogReg(0x90, 0x40); //an_tp_cyc set 1
	WriteAnalogReg(0xa5, 0x4c); //tx_mod[6] tx cyc1[2] manual
	WriteAnalogReg(0x9e,0x00) ; //reg_dc set 0 fro tpcal otherwise, carryover for intgN
	WRITE_REG8(0x4e8,0x4c);

	if(mode==RF_MODE_BLE_2M)
		sub_wr_ana(0x8f, 0x12, 4, 0);
	else
		sub_wr_ana(0x8f, 0x0b, 4, 0);

	WriteAnalogReg(0x93, pGain);
	sub_wr(0x400,2,6,5);

	WaitMs(300);
	cnt = tpGetCnt(precision);
	cntExp = cnt + vcnt*fdev/48000;
	sub_wr(0x400,1,5,5);

	for(ii=0; ii<=5; ii++)
	{
		WaitMs(10);
		cnt = tpGetCnt(precision);
		iGain = 1<<(5-ii);
		 ReadAnalogReg( 0x93);

		if(cnt > cntExp)
		{
			pGain = pGain -iGain;
			WriteAnalogReg(0x93, pGain);
		}
    	else if (cnt < cntExp)
    	{
       		pGain = pGain + iGain;
			WriteAnalogReg(0x93, pGain); 	//initial gain
    	}
	}

	WriteAnalogReg(0x90, 0x00); 		//an_tp_cyc set 0
	WRITE_REG8(0x4e8, 0x0c);		//txmod[6] set 0
	sub_wr_ana(0xab, 0x0, 1, 0);//turn on PA

	return  (ReadAnalogReg( 0x93) & 0x7f);
}

static void rf_tx_mode(RF_ModeTypeDef RF_Mode)
{
//	if ((RF_Mode==RF_MODE_BLE_1M) | (RF_Mode==RF_MODE_BLE_1M_NO_PN))
	if (RF_Mode==RF_MODE_BLE_1M)
	{
		WriteAnalogReg(0x9e,0x56); //reg_dc *128/126
		WriteAnalogReg(0xa3,0xeb); //[7:6] diable gauflt [5] LUT 2M or 1M
		WRITE_REG8(0x404, 0xd0);//ble1m[4]
	}
	else if(RF_Mode==RF_MODE_BLE_2M)
	{
		WriteAnalogReg(0x9e,0xad); //reg_dc *128/126
		WriteAnalogReg(0xa3,0xdb);  //[7:6] diable gauflt [5] LUT 2M or 1M TODO  to check
		WRITE_REG8(0x404, 0xc0);//ble1m[4]
	}
	WriteAnalogReg(0x90,0x00); //tpcyc
	WriteAnalogReg(0xa5,0x44); //mod_en
	WRITE_REG8(0x4e8, 0x44);//mod_en
	WriteAnalogReg(0xa0,0x03); //[4:0] dac delay
}

int pnGen(int state)
{
	int feed = 0;
	feed = (state&0x4000) >> 1;
	state ^= feed;
	state <<= 1;
	state = (state&0xfffe) + ((state&0x8000)>>15);
	return state;
}

static void rf_continue_setting(RF_ModeTypeDef mode)
{
	unsigned char i;


	WRITE_REG8 (0x400,0x0f);//0b for 2Mbps, 03 for Zigbee, 0f for New2M and BLE Nrd
	WRITE_REG8(0x402, 0x21);

	WRITE_REG16(0x50c, (unsigned short*)pkt_phytest); //set DMA addr
	WRITE_REG8(0x50e, 0x01);  //size
	WRITE_REG8(0x50f, 0x80); //mode

	state0 = STATE0;
	state1 = STATE1;

	pkt_phytest[0] = 12;
	pkt_phytest[1] = 0;
	pkt_phytest[2] = 0;
	pkt_phytest[3] = 0;

	for(i=0;i<16;i= i+4)
	{
		pkt_phytest[4+i] = state1 &0xff;
		pkt_phytest[5+i] = (state1>>8) &0xff;
		pkt_phytest[6+i] = state0 &0xff;
		pkt_phytest[7+i] = (state0>>8) &0xff;

		state0 = pnGen(state0);
		state1 = pnGen(state1);
	}

	state0 = STATE0;
	state1 = STATE1;
	state2 = STATE2;
	state3 = STATE3;

	WRITE_REG8(0x524,0x08);
}

void rf_emi_tx_continue_setup(RF_ModeTypeDef rf_mode, RF_TxPowerTypeDef power_level,signed char rf_chn,unsigned char pkt_type)
{

	WRITE_REG8(0x4e8, 0x02);
	WriteAnalogReg(0xa5,0x00);

	//reset zb & dma
	write_reg16(0x800060, 0x0480);
	write_reg16(0x800060, 0x0000);

	rf_set_power_level_index (power_level);
	//rf_drv_init(rf_mode);

	rf_set_channel(rf_chn, 0);
	rf_set_txmode();

	WRITE_REG16(0x408, pkt_type);
	rf_continue_setting(rf_mode);

}

void rf_continue_mode_run(void)
{
	if(read_reg8(0x408) == 1)
	{
		pkt_phytest[12] = 0x0f;
		pkt_phytest[13] = 0x0f;
		pkt_phytest[14] = 0x0f;
		pkt_phytest[15] = 0x0f;
	}
	else if(read_reg8(0x408)==2)
	{
		pkt_phytest[12] = 0x55;
		pkt_phytest[13] = 0x55;

		pkt_phytest[14] = 0x55;
		pkt_phytest[15] = 0x55;
	}
	else
	{
		pkt_phytest[12] = state1 &0xff;
		pkt_phytest[13] = (state1>>8) &0xff;

		pkt_phytest[14] = state0 &0xff;
		pkt_phytest[15] = (state0>>8) &0xff;

		state0 = pnGen(state0);
		state1 = pnGen(state1);
	}
}

void rf_emi_tx_brust_setup(RF_ModeTypeDef rf_mode,RF_TxPowerTypeDef power_level,signed char rf_chn,unsigned char pkt_type)
{
	unsigned char i;
	unsigned char tx_data=0;
	WRITE_REG8 (0x402, 0x2b);
	WriteAnalogReg(0xa5,0x00);
	WRITE_REG8 (0x4e8, 0x02);

	write_reg32(0x408,0x29417671 );//access code  0xf8118ac9
	rf_set_power_level_index (power_level);

	WRITE_REG8 (0x50e, 0xff); // dma size
	WRITE_REG8 (0x50f, 0x80); // dma mode

	//rf_drv_init(rf_mode);
//	rf_set_channel(rf_chn,0);

	if(pkt_type==1)
		tx_data = 0x0f;
	else if(pkt_type==2)
		tx_data = 0x55;


	switch(rf_mode)
	{
		case RF_MODE_BLE_1M:
		case RF_MODE_BLE_2M:
			emi_ble_tx_packet[4] = pkt_type;//type
			for( i=0;i<37;i++)
			{
				emi_ble_tx_packet[6+i]=tx_data;
			}
			break;
		default:
			break;
	}
	rf_set_trx_state(RF_MODE_AUTO, rf_chn);
}

static void gen_prbs9 (unsigned char *p, int n)
{
	//PRBS9: (x >> 1) | (((x<<4) ^ (x<<8)) & 0x100)
	unsigned short x = 0x1ff;
	int i;
	int j;
	for ( i=0; i<n; i++)
	{
		unsigned char d = 0;
		for (j=0; j<8; j++)
		{
			if (x & 1)
			{
				d |= BIT(j);
			}
			x = (x >> 1) | (((x<<4) ^ (x<<8)) & 0x100);
		}
		*p++ = d;
	}
}

void rf_emi_tx_brust_loop(RF_ModeTypeDef rf_mode,unsigned char pkt_type)
{
	write_reg8(0xf00, 0x80); // stop SM

	if((rf_mode==RF_MODE_BLE_1M)||(rf_mode==RF_MODE_BLE_2M))//ble
	{
		rf_start_stx ((void *)emi_ble_tx_packet, read_reg32(0x740) + 10);
		WaitUs(625);//
		if(pkt_type==0)
			gen_prbs9(&emi_ble_tx_packet[6],37);
	}
}

void rf_emi_rx(RF_ModeTypeDef mode,signed char rf_chn)
{

	WriteAnalogReg(0xa5,0x00);
	WRITE_REG8 (0x4e8, 0x02);

	rf_set_ble_access_code_value(0x29417671);
	rf_set_rx_buff(emi_rx_packet,64,0);
//	rf_drv_init(mode);
//	rf_set_channel(rf_chn,0);//set freq
//	rf_set_rxmode();

	rf_set_trx_state(RF_MODE_RX, rf_chn);
	rssi = 0;
	emi_rssibuf = 0;
	emi_rx_cnt = 0;
}

void rf_emi_rx_loop(void)
{
	if(read_reg8(0xf20)&BIT(0))
	{
		if((REG_ADDR8(0x45c)&0x0f)==0)
		{
			emi_rssibuf += emi_rx_packet[4];
			if(emi_rx_cnt)
			{
				if(emi_rssibuf!=0)
					emi_rssibuf>>=1;
			}
			rssi = emi_rssibuf-110;
			emi_rx_cnt++;
		}
		write_reg8(0x800f20, 1);
		write_reg8 (0x800f00, 0x80);
	}
}

unsigned int rf_emi_get_rxpkt_cnt(void)
{
	return emi_rx_cnt;
}

char rf_emi_get_rssi_avg(void)
{
	return rssi;
}
