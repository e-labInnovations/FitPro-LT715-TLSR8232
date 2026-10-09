/********************************************************************************************************
 * @file     rf_ll.c
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
 * @date     August. 30, 2019
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
#include "drivers.h"
#include "rf_ll.h"
#include "../common/rf_frame.h"

#ifndef			CHANNEL_SLOT_TIME
#define			CHANNEL_SLOT_TIME			8000
#endif

#define		PKT_BUFF_SIZE					64

#define		LL_CHANNEL_SYNC_TH				2
#define		LL_CHANNEL_SEARCH_TH			60
#define		LL_CHANNEL_SEARCH_FLAG			BIT(16)
#define		LL_NEXT_CHANNEL(c)				((c + 6) & 14)


unsigned char  		rf_rx_buff[PKT_BUFF_SIZE*2] __attribute__((aligned(4)));

u8		host_channel 	= 0;
u8		chn_mask 		= 0x80;

u32		ll_chn_mask 	= LL_CHANNEL_SEARCH_FLAG;
int		device_sync 	= 0;

u8		device_channel;
u8		ll_chn_sel;
u8		ll_rssi;

u16		ll_chn_tick;

u32		ll_chn_rx_tick;
u32		ll_clock_time;

int		rf_rx_wptr;

unsigned int		cpu_wakup_last_tick;
volatile int		device_ack_received = 0;

extern rf_packet_pairing_t	pkt_pairing;


/////////////////////////////////////////////////////////////////////
void rf_set_access_code_pairing(unsigned int code){ // for pairing
	WRITE_REG32 (0x800408, 0x71 | (code & 0xffffff00));
	WRITE_REG8  (0x80040c, code);
}

void rf_set_access_code_data(unsigned int code){ // for data
	WRITE_REG32 (0x800408, 0x03 | (code & 0xffffff00));
	WRITE_REG8  (0x80040c, code);
}

void rf_receiving_pipe_enble(u8 channel_mask)
{
	WRITE_REG8  (0x800407, channel_mask);	// channel mask
}

u8	get_next_channel_with_mask(u32 mask, u8 chn)
{
	int chn_high = (mask >> 4) & 0x0f;

	if (mask & LL_CHANNEL_SEARCH_FLAG) {
		return LL_NEXT_CHANNEL (chn);
	}
	else if (chn_high != chn) {
		ll_chn_sel = 1;
		return chn_high;
	}
	else {
		ll_chn_sel = 0;
		return mask & 0x0f;
	}
}

void ll_device_init (void)
{
	reg_dma_rf_rx_addr = (u16)(u32) (rf_rx_buff);
	reg_dma2_ctrl = FLD_DMA_WR_MEM | (PKT_BUFF_SIZE>>4);   // rf rx buffer enable & size
	reg_dma_chn_irq_msk = 0;
	reg_irq_mask |= FLD_IRQ_ZB_RT_EN;    //enable RF & timer1 interrupt
	reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX;
}

_attribute_ram_code_ void rf_send_packet (void* addr, unsigned short rx_waittime, unsigned char retry)
{
	analog_write (0x06, 0);
	WRITE_REG8  (0x800f00, 0x80);				// stop
	WRITE_REG8  (0x800f14, retry);				// number of retry
	WRITE_REG16 (0x80050c, (unsigned short)((unsigned int)addr));
	WRITE_REG16 (0x800f0a, rx_waittime);
	WRITE_REG16 (0x800f00, 0x3f83);
}

_attribute_ram_code_ void irq_device_rx(void)
{
	static u32 irq_device_rx_no = 0;
	u8 * raw_pkt = (u8 *) (rf_rx_buff + rf_rx_wptr * PKT_BUFF_SIZE);
	rf_rx_wptr = (rf_rx_wptr + 1) & 1;
	reg_dma_rf_rx_addr = (u16)(u32) (rf_rx_buff + rf_rx_wptr * PKT_BUFF_SIZE); //set next buffer

	reg_rf_irq_status = FLD_RF_IRQ_RX;

	if	(	raw_pkt[0] >= 15 &&
			RF_PACKET_2M_LENGTH_OK(raw_pkt) &&
			RF_PACKET_CRC_OK(raw_pkt) )	{
		DBG_CHN2_TOGGLE;
		rf_packet_ack_pairing_t *p = (rf_packet_ack_pairing_t *)(raw_pkt + 8);
		rf_set_tx_rx_off_auto_mode();
		extern int  rf_rx_process(u8 *);
		if (rf_rx_process (raw_pkt) && ll_chn_tick != p->tick) {
			ll_chn_tick = p->tick;			//sync time
#if 0
			device_sync = 1;
#endif
			device_ack_received = 1;
			ll_chn_mask = p->chn;			//update channel
			ll_chn_rx_tick = clock_time ();
			ll_rssi = raw_pkt[4];
			irq_device_rx_no++;
		}
		rf_set_channel (device_channel, RF_CHN_TABLE);
		raw_pkt[0] = 1;
	}
}

_attribute_ram_code_ void irq_device_tx(void)
{
	reg_rf_irq_status = FLD_RF_IRQ_TX;
}

_attribute_ram_code_ int	device_send_packet (u8 * p, u32 timeout, int retry, int pairing_link)
{
	extern u32  cpu_wakup_last_tick;
    while ( !clock_time_exceed (cpu_wakup_last_tick, 500) );    //delay to get stable pll clock


	static	u32 ack_miss_no;

	device_ack_received = 0;
	int i;
	int step = 1;
	if (device_sync)
		step = retry + 1;
	for (i=0; i<=retry; i += step) {
		DBG_CHN3_TOGGLE;
		rf_set_channel (device_channel, RF_CHN_TABLE);
		u32 t = clock_time ();
		rf_send_packet (p, 300, step - 1);
		reg_rf_irq_status = 0xffff;

		while (	!device_ack_received &&
				!clock_time_exceed (t, timeout*step) &&
				!(reg_rf_irq_status & (FLD_RF_IRX_RETRY_HIT | FLD_RF_IRX_CMD_DONE)) );

		if (device_ack_received) {
			ack_miss_no = 0;
			break;
		}
		ack_miss_no ++;
		if (ack_miss_no >= LL_CHANNEL_SEARCH_TH) {
			device_sync = 0;
			ll_chn_mask = LL_CHANNEL_SEARCH_FLAG;
		}
		else if (ack_miss_no >= LL_CHANNEL_SYNC_TH) {
			device_sync = 0;
		}

		if (!device_sync) {	//alternate channel while device not in sync mode
			device_channel = get_next_channel_with_mask (ll_chn_mask, device_channel);
		}
	}

	analog_write (0x06, 0xff);

	if (i <= retry) {
		return 1;
	}
	else{
		return 0;
	}
}

void ll_add_clock_time (u32 ms)
{
	if (ms > CHANNEL_SLOT_TIME * 8) {
		device_sync = 0;
		ll_clock_time = 0;
	}
	else {
		ll_clock_time += ms;
		while (ll_clock_time >= CHANNEL_SLOT_TIME) {
			ll_clock_time -= CHANNEL_SLOT_TIME;
			device_channel = get_next_channel_with_mask (ll_chn_mask, device_channel);
		}
	}

}




